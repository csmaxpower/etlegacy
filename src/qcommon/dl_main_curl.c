/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file dl_main_curl.c
 *
 * @todo Additional features that would be nice for this code:
 *           Only display \<gamepath\>/\<file\>, e.g., etpro/etpro-3_0_1.pk3 in the UI.
 *           Add server as referring URL
 */

#include <curl/curl.h>

#include "q_shared.h"
#include "qcommon.h"
#include "dl_public.h"

#ifdef FEATURE_SSL

#if defined(USING_WOLFSSL) || defined(USING_OPENSSL)
	#define SSL_VERIFY 1
#endif

#if SSL_VERIFY

#ifdef USING_WOLFSSL
#   define OPENSSL_ALL 1
#   include <wolfssl/options.h>
#   include <wolfssl/ssl.h>
#endif

#ifdef USING_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#endif
#endif

#define APP_NAME        "ID_DOWNLOAD"
#define APP_VERSION     "2.0"

#define GET_BUFFER_SIZE (1024 * 256)

#ifdef __linux__
#define CA_CERT_DEFAULT "/etc/ssl/certs/ca-certificates.crt"
#endif

// Helper macro to output the possible error message to the shared log output
#define ETL_curl_easy_setopt(status, handle, opt, param) \
		if (((status) = curl_easy_setopt((handle), (opt), (param)))) \
		Com_Printf(S_COLOR_YELLOW "WARNING: %s: curl_easy_setopt " #opt ": %s\n", __func__, curl_easy_strerror(status))

//static CURLM *dl_multi   = NULL;
//static CURL  *dl_request = NULL;
//static FILE  *dl_file    = NULL;

static struct
{
	qboolean initialized;   ///< the main initialization flag (Initialize once)
	qboolean abort;        ///< abort all running requests

	webRequest_t *requests;
	unsigned int requestId;

	dlStatus_t packDownloadStatus;
	CURL *fHandle;          ///< file download handle which is just by the vanilla server download
	CURLM *multiHandle;     ///< main curl multi request handle
} webSys = { qfalse, qfalse, NULL, 0, DL_DONE, NULL, NULL };

#if defined(FEATURE_SSL) && SSL_VERIFY
static CURLcode DL_cb_Context(CURL *curl, void *ssl_ctx, void *parm)
{
	fileHandle_t certHandle;
	int          i;
	(void)curl;
	(void)parm;

	int len = (int) FS_SV_FOpenFileRead(CA_CERT_FILE, &certHandle);
	if (len <= 0)
	{
		FS_FCloseFile(certHandle);
		goto callback_failed;
	}

	char *buffer = Com_Allocate(len + 1);
	buffer[len] = 0;
	FS_Read(buffer, len, certHandle);
	FS_FCloseFile(certHandle);

	BIO        *cbio = BIO_new_mem_buf(buffer, len);
	X509_STORE *cts  = SSL_CTX_get_cert_store((SSL_CTX *)ssl_ctx);
	STACK_OF(X509_INFO) * inf;

	if (!cts || !cbio)
	{
		Com_Dealloc(buffer);
		goto callback_failed;
	}

	inf = PEM_X509_INFO_read_bio(cbio, NULL, NULL, NULL);

	if (!inf)
	{
		BIO_free(cbio);
		Com_Dealloc(buffer);
		goto callback_failed;
	}

	for (i = 0; i < sk_X509_INFO_num(inf); i++)
	{
		X509_INFO *itmp = sk_X509_INFO_value(inf, i);
		if (itmp->x509)
		{
			X509_STORE_add_cert(cts, itmp->x509);
		}
		if (itmp->crl)
		{
			X509_STORE_add_crl(cts, itmp->crl);
		}
	}

	sk_X509_INFO_pop_free(inf, X509_INFO_free);
	BIO_free(cbio);
	Com_Dealloc(buffer);

	return CURLE_OK;

callback_failed:
	return CURLE_ABORTED_BY_CALLBACK;
}
#endif

/**
 * @brief DL_cb_FWriteFile
 * @param[in] ptr
 * @param[in] size
 * @param[in] nmemb
 * @param[in] userp
 * @return
 */
static size_t DL_cb_FWriteFile(void *ptr, size_t size, size_t nmemb, void *userp)
{
	webRequest_t *request = (webRequest_t *)userp;

	if (webSys.abort || request->abort)
	{
		return -666;
	}

	return fwrite(ptr, size, nmemb, request->data.fileHandle);
}

/**
 * @brief DL_cb_Progress
 * @param clientp - unused
 * @param dltotal - unused
 * @param[in] dlnow
 * @param ultotal - unused
 * @param ulnow   - unused
 * @return
 *
 * @note cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
 * and it would probably be expensive to verify them here.
 */
static int DL_cb_Progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	int          resp     = 0;
	webRequest_t *request = (webRequest_t *)clientp;

	if (webSys.abort || request->abort)
	{
		return -666;
	}

	if (request->data.requestLength)
	{
		if (request->upload)
		{
			request->data.requestLength = (size_t)ultotal;
		}
		else
		{
			request->data.requestLength = (size_t)dltotal;
		}
	}

	if (request->progress_clb)
	{
		if (request->upload)
		{
			resp = request->progress_clb(request, ulnow, ultotal);
		}
		else
		{
			resp = request->progress_clb(request, dlnow, dltotal);
		}
	}
	return resp;
}

/**
 * @brief DL_write_function
 * @param[in] ptr
 * @param[in] size
 * @param[in] nmemb
 * @param[out] stream
 * @return
 */
size_t DL_write_function(void *ptr, size_t size, size_t nmemb, void *userp)
{
	webRequest_t *request = (webRequest_t *)userp;

	if (webSys.abort || request->abort)
	{
		return -666;
	}

	if (request->data.bufferPos + size * nmemb >= GET_BUFFER_SIZE - 1)
	{
		Com_Printf(S_COLOR_RED  "DL_write_function: Error - buffer is too small");
		return 0;
	}

	Com_Memcpy(request->data.buffer + request->data.bufferPos, ptr, size * nmemb);
	request->data.bufferPos += size * nmemb;

	return size * nmemb;
}

/**
 * @brief DL_InitDownload
 */
void DL_InitDownload(void)
{
	if (webSys.initialized)
	{
		return;
	}

	Com_Printf("Version %s\n", curl_version());

	/* Make sure curl has initialized, so the cleanup doesn't get confused */
	curl_global_init(CURL_GLOBAL_ALL);

	webSys.multiHandle = curl_multi_init();

	Com_Printf("Client download subsystem initialized\n");
	webSys.initialized = qtrue;
	webSys.abort       = qfalse;
}

/**
 * @brief DL_Shutdown
 */
void DL_Shutdown(void)
{
	if (!webSys.initialized)
	{
		return;
	}

	DL_AbortAll(qtrue, qfalse);

	curl_multi_cleanup(webSys.multiHandle);
	webSys.multiHandle = NULL;

	curl_global_cleanup();

	webSys.initialized = qfalse;
}

/**
 * @brief gracefully abort all active requests
 * @attention this will block for a possibly multiple ms.
 */
void DL_AbortAll(qboolean block, qboolean allowContinue)
{
	int limit = 20;

	// let the handles die off gracefully if any are still active
	webSys.abort = qtrue;

	if (block)
	{
		while (limit && webSys.requests)
		{
			DL_DownloadLoop();
			limit--;
		}
	}

	if (!webSys.requests && allowContinue)
	{
		webSys.abort = qfalse;
	}
}

/**
 * Setup ssl verification for the request object
 * @param curl request object
 */
static void DL_InitSSL(CURL *curl)
{
#if defined(FEATURE_SSL)
	CURLcode status;

#if defined(USING_SCHANNEL)
	ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 1);
	ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 1);
#elif SSL_VERIFY
	ETL_curl_easy_setopt(status, curl, CURLOPT_CAINFO, NULL);
	ETL_curl_easy_setopt(status, curl, CURLOPT_CAPATH, NULL);

	if (FS_SV_FileExists(CA_CERT_FILE, qtrue))
	{
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_CTX_FUNCTION, DL_cb_Context);
	}
	else if (FS_FileInPathExists(Cvar_VariableString("dl_capath")))
	{
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_CAINFO, Cvar_VariableString("dl_capath"));
	}
#ifdef __linux__
	else if (FS_FileInPathExists(CA_CERT_DEFAULT))
	{
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_CAINFO, CA_CERT_DEFAULT);
	}
#endif
	else
	{
#if defined(_WIN32) && defined(USING_OPENSSL)
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 1);
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#else
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 0);
		ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif
	}
#else
	ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYHOST, 0);
	ETL_curl_easy_setopt(status, curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif
#endif
}

static unsigned int DL_GetRequestId()
{
	while (qtrue)
	{
		unsigned int tmp = 1 + (++webSys.requestId);

		// 0 is an invalid id, and 1 is reserved
		if (tmp == 0 || tmp == 1)
		{
			continue;
		}

		// wrap around protection
		// very highly unlikely that this ever happens, but you never know
		webRequest_t **lst = &webSys.requests;

		while (*lst)
		{
			if ((*lst)->id == tmp)
			{
				tmp = 0;
				break;
			}

			lst = &(*lst)->next;
		}

		// so we found the id from the existing request, try again..
		if (!tmp)
		{
			continue;
		}

		return tmp;
	}
}

static webRequest_t *DL_CreateRequest()
{
	webRequest_t *request = Com_Allocate(sizeof(webRequest_t));
	if (!request)
	{
		Com_Error(ERR_FATAL, "Cannot allocate memory for the request\n");
		return NULL;
	}
	Com_Memset(request, 0, sizeof(webRequest_t));
	request->id = DL_GetRequestId();

	request->next   = webSys.requests;
	webSys.requests = request;

	return request;
}

static void DL_FreeRequest(webRequest_t *request)
{
	webRequest_t **lst = &webSys.requests;

	// pop from the list
	while (*lst)
	{
		if (*lst == request)
		{
			*lst = request->next;
			break;
		}
	}

	if (request->data.fileHandle)
	{
		// Com_Printf(S_COLOR_YELLOW "WARNING: file handle was not closed by callback\n");
		fclose(request->data.fileHandle);
		request->data.fileHandle = NULL;
	}

	if (request->data.buffer)
	{
		// Com_Printf(S_COLOR_YELLOW "WARNING: buffer was not freed by callback\n");
		Com_Dealloc(request->data.buffer);
		request->data.buffer = NULL;
	}

	if (request->rawHandle)
	{
		curl_easy_cleanup(request->rawHandle);
		request->rawHandle = NULL;
	}

	Com_Dealloc(request);
}

/**
 * @brief Inspired from http://www.w3.org/Library/Examples/LoadToFile.c
 * setup the download, return once we have a connection
 *
 * @param localName
 * @param remoteName
 * @return
 */
unsigned int DL_BeginDownload(const char *localName, const char *remoteName, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	char         referer[MAX_STRING_CHARS + 5 /*"et://"*/];
	CURLcode     status;
	webRequest_t *request;

	if (webSys.fHandle)
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - called with a download request already active\n");
		return 0;
	}

	if (!localName[0] || !remoteName[0])
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - empty download URL or empty local file name\n");
		return 0;
	}

	if (FS_CreatePath(localName))
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - unable to create directory (%s).\n", localName);
		return 0;
	}

	request     = DL_CreateRequest();
	request->id = 1; // magical package download id

	request->data.fileHandle = Sys_FOpen(localName, "wb");
	if (!request->data.fileHandle)
	{
		Com_Printf(S_COLOR_RED  "DL_BeginDownload: Error - unable to open '%s' for writing\n", localName);
		DL_FreeRequest(request);
		return 0;
	}

	DL_InitDownload();

	/* ET://ip:port */
	strcpy(referer, "et://");
	Q_strncpyz(referer + 5, Cvar_VariableString("cl_currentServerIP"), MAX_STRING_CHARS);

	webSys.fHandle = request->rawHandle = curl_easy_init();

	request->complete_clb = complete;
	request->progress_clb = progress;

	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_USERAGENT, va("%s %s", APP_NAME "/" APP_VERSION, curl_version()));
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_REFERER, referer);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_URL, remoteName);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_WRITEDATA, (void *)request);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_PROGRESSFUNCTION, DL_cb_Progress);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_PROGRESSDATA, (void *)request);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_NOPROGRESS, 0);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_FAILONERROR, 1);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_FOLLOWLOCATION, 1);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_MAXREDIRS, 5);

	DL_InitSSL(request->rawHandle);

	if (curl_multi_add_handle(webSys.multiHandle, request->rawHandle) != CURLM_OK)
	{
		Com_Printf(S_COLOR_RED  "DL_BeginDownload: Error - invalid handle.\n");

		if (request->complete_clb)
		{
			request->complete_clb(request, qfalse);
		}
		DL_FreeRequest(request);
		return 0;
	}

	Cvar_Set("cl_downloadName", remoteName);

	return request->id;
}

/**
 * @brief Web_CreateRequest
 * @param[in] url Full web url
 * @param[in] get Get or Post request
 * @param[in] complete complete callback function
 * @param[in] progress progress callback function
 * @return request creation successful
 *
 * @note Unused
 */
int Web_CreateRequest(const char *url, qboolean get, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	CURLcode     status;
	webRequest_t *request;

	if (!url)
	{
		Com_Printf(S_COLOR_RED "DL_GetString: Error - empty download URL\n");
		return qfalse;
	}

	DL_InitDownload();

	request = DL_CreateRequest();

	request->complete_clb = complete;
	request->progress_clb = progress;

	request->rawHandle = curl_easy_init();

	request->data.buffer = Com_Allocate(GET_BUFFER_SIZE);
	if (!request->data.buffer)
	{
		goto error_get;
	}
	request->data.bufferSize = GET_BUFFER_SIZE;
	Com_Memset(request->data.buffer, 0, GET_BUFFER_SIZE);

	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_USERAGENT, va("%s %s", APP_NAME "/" APP_VERSION, curl_version()));
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_URL, url);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_FOLLOWLOCATION, 1L);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_MAXREDIRS, 5);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_WRITEFUNCTION, DL_write_function);
	ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_WRITEDATA, (void *)&request);

	if (!get)
	{
		ETL_curl_easy_setopt(status, request->rawHandle, CURLOPT_POST, 1L);
	}

	DL_InitSSL(request->rawHandle);

	if (curl_multi_add_handle(webSys.multiHandle, request->rawHandle) != CURLM_OK)
	{
		Com_Printf(S_COLOR_RED  "DL_GetString: Error - invalid handle.\n");
		goto error_get;
	}

	return qtrue;

error_get:

	if (request->complete_clb)
	{
		request->complete_clb(request, qfalse);
	}
	DL_FreeRequest(request);

	return qfalse;
}

static void DL_SetupContentLength(CURL *handle)
{
	webRequest_t **lst = &webSys.requests;

	while (*lst)
	{
		webRequest_t *req = *lst;
		if (handle == req->rawHandle)
		{
			if (!req->data.requestLength)
			{
				curl_off_t cl;
				if (curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl) == CURLE_OK)
				{
					if (cl != -1)
					{
						req->data.requestLength = cl;
						if (handle == webSys.fHandle)
						{
							Cvar_SetValue("cl_downloadSize", (float) cl);
						}
					}
				}
			}
		}
		lst = &req->next;
	}
}

/**
 * @brief DL_DownloadLoop
 */
void DL_DownloadLoop(void)
{
	CURLMcode  status;
	CURLMsg    *msg;
	int        dls  = 0;
	const char *err = NULL;

	if (!webSys.initialized)
	{
		return;
	}

	if ((status = curl_multi_perform(webSys.multiHandle, &dls)) == CURLM_CALL_MULTI_PERFORM && dls)
	{
		return;
	}

	if (status > CURLM_OK)
	{
		err = curl_multi_strerror(status);
		//FIXME: maybe clear the multi handle and free all downloads?
		// can we just ignore this? ..needs testing.
		Com_Error(ERR_FATAL, "Multi-handle error %s\n", err);
	}

	while ((msg = curl_multi_info_read(webSys.multiHandle, &dls)))
	{
		if (msg->msg != CURLMSG_DONE)
		{
			DL_SetupContentLength(msg->easy_handle);
			continue;
		}

		CURL         *handle   = msg->easy_handle;
		webRequest_t **lst     = &webSys.requests;
		qboolean     requestOk = msg->data.result == CURLE_OK;

		// before doing anything else, remove the request from the multi-handle
		curl_multi_remove_handle(webSys.multiHandle, handle);

		if (!requestOk)
		{
			err = curl_easy_strerror(msg->data.result);
			Com_Printf(S_COLOR_RED "DL_DownloadLoop: Error - request terminated with failure status '%s'\n", err);
		}

		if (handle == webSys.fHandle)
		{
			webSys.packDownloadStatus = (requestOk ? DL_DONE : DL_FAILED);
			webSys.fHandle            = NULL;
		}

		while (*lst)
		{
			webRequest_t *req = *lst;
			if (handle == req->rawHandle)
			{
				handle = NULL;

				// do not care about callbacks if we are force cancelling
				if (webSys.abort || req->abort)
				{
					DL_FreeRequest(req);
					break;
				}

				// If we have been using a file handle then close it before calling the callback..
				if (req->data.fileHandle)
				{
					fclose(req->data.fileHandle);
					req->data.fileHandle = NULL;
				}

				if (req->complete_clb)
				{
					req->complete_clb(req, requestOk);
				}

				DL_FreeRequest(req);

				break;
			}

			lst = &req->next;
		}

		if (handle)
		{
			Com_Printf(S_COLOR_YELLOW "Download handle was not closed properly, closing it now.\n");
			curl_easy_cleanup(handle);
		}
	}
}
