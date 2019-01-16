

/* This is a simple example showing how to send mail using libcurl's SMTP
 * capabilities. It builds on the smtp-mail.c example to add authentication
 * and, more importantly, transport security to protect the authentication
 * details from being snooped.
 *
 * Note that this example requires libcurl 7.20.0 or above.
 */

#define TO      "<99ezequiel90@gmail.com>"
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#define size 1000

#define FROM    "<sender@example.org>"
#define CC      "<info@example.org>"


static const char *payload_text[] = {
  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
  "To: " TO "\r\n",
  "From: " FROM " (Example User)\r\n",
  "Cc: " CC " (Another example User)\r\n",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
  "rfcpedant.example.org>\r\n",
  "Subject: SMTP TLS example message\r\n",
  "\r\n", 
  "The body of the message starts here.\r\n",
  "\r\n",
  "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
  "Check RFC5322.\r\n",
  NULL
};

static const char *payload_text_code_send_example[] = {
  "Date: Wednesday, 16 Jan 2019 14:00:00 +0000\r\n",
  "To: " TO "\r\n",
  "From: " FROM " (Example User)\r\n",
  "Cc: " CC " (ignore this)\r\n",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
  "rfcpedant.example.org>\r\n",
  "Subject: Security Code message\r\n",
  "\r\n", 
  "Your security code is 332211.\r\n",
  "\r\n",
  "This is just a test, not an actual valid code\r\n",
  "And this should be going where it should, hopefully\r\n",
  NULL
};

struct upload_status {
  int lines_read;
};




//exemplo de função que lê o input para o payload

static size_t payload_source(void *ptr, size_t sizePayload, size_t nmemb, void *userp){
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;

  if((sizePayload == 0) || (nmemb == 0) || ((sizePayload*nmemb) < 1)) {
    return 0;
  }

  //adicionar data à mensagem
  //data = payload_text[upload_ctx->lines_read];
  data = payload_text_code_send_example[upload_ctx->lines_read];
  
  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    upload_ctx->lines_read++;

    return len;
  }

  return 0;
}


int stuff(void){

  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;

  upload_ctx.lines_read = 0;

  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */

    //nota: isto corresponde a um email da google
    //portanto devo criar um novo e colcoar aqui as credenciais associadas
    curl_easy_setopt(curl, CURLOPT_USERNAME, "tp3ssiezenad");
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "palavrapassesegura");

    /* This is the URL for your mailserver. Note the use of port 587 here,
     * instead of the normal SMTP port (25). Port 587 is commonly used for
     * secure mail submission (see RFC4403), but you should use whatever
     * matches your server configuration. */

    //nota: estamos a usar o serviço gmail, logo devemos utilizar este servidor
    //para mais informaçoes ir a 
    //https://stackoverflow.com/questions/37092597/sending-an-email-with-libcurl-smtp-with-gmail-login-denied
    //e https://support.google.com/a/answer/176600?hl=en
    curl_easy_setopt(curl, CURLOPT_URL, "smtp.gmail.com:587");

    /* In this example, we'll start with a plain text connection, and upgrade
     * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
     * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
     * will continue anyway - see the security discussion in the libcurl
     * tutorial for more details. */
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    /* If your server doesn't have a valid certificate, then you can disable
     * part of the Transport Layer Security protection by setting the
     * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false).
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
     * That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.  Instead, you should get
     * the issuer certificate (or the host certificate if the certificate is
     * self-signed) and add it to the set of certificates that are known to
     * libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See docs/SSLCERTS
     * for more information. */

    //retirado de https://curl.haxx.se/docs/caextract.html

    curl_easy_setopt(curl, CURLOPT_CAINFO, "./cacert.pem");

    /* Note that this option isn't strictly required, omitting it will result
     * in libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */
    recipients = curl_slist_append(recipients, TO);
    recipients = curl_slist_append(recipients, CC);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */

    //nota: aqui é que ele coloca o payload
    //pelo que alterar a mensagem será alterar o payload_source
    //para algo diferente

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);

    

    
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);


    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the transfer.
     */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* Send the message */
    res = curl_easy_perform(curl);

    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* Free the list of recipients */
    curl_slist_free_all(recipients);

    /* Always cleanup */
    curl_easy_cleanup(curl);
  }

  return (int)res;
}
