// diretamente retirado de 
// https://raw.githubusercontent.com/curl/curl/master/docs/examples/smtp-tls.c
// retirando um comentário


//A ideia deste é testar se consegue mandar um email para
//a minha conta gmail sem problemas

//será alterado do exemplo original até conseguir enviar


/* <DESC>
 * SMTP example using TLS
 * </DESC>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define SIZE 1000

#define FROM    "<sender@example.org>"


struct upload_status {
  int lines_read;
  char *payload[];
};


//função para ler o payload criado 


static size_t payload_source_custom(void *ptr, size_t sizePayload, size_t nmemb, void *userp){
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;

  if((sizePayload == 0) || (nmemb == 0) || ((sizePayload*nmemb) < 1)) {
    return 0;
  }

  //adicionar data à mensagem
  //relembrar arrays em c
  data = upload_ctx->payload[upload_ctx->lines_read];
  
  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    upload_ctx->lines_read++;

    return len;
  }

  return 0;
}


//função cujo objetivo é mandar a target(um endereço email) um código

int sendMailToSomeoneWithACode(char* target,char* codeToSend,char* pathToFolder){
  char destinatario[SIZE] ;
  char cc[SIZE];
  char codigo[SIZE];


  
  //criar destinatário
  //strcpy(destinatario,"<");
  strcpy(destinatario,target);
  //strcat(destinatario,">");


  //criar cc
  //strcpy(cc,"<");
  strcpy(cc,target);
  //strcat(cc,">");




  //criar mensagem
  strcpy(codigo,"Your security code is ");
  strcat(codigo,codeToSend);
  strcat(codigo,".\r\n");

  printf("%s\n%s\n",destinatario,codigo);

  char *payload_to_send[] = {
    "Date: Wednesday, 16 Jan 2019 14:00:00 +0000\r\n",
    destinatario,
    "From: " FROM " (Example User)\r\n",
    "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
    "rfcpedant.example.org>\r\n",
    "Subject: Security Code message\r\n",
    "\r\n", /* empty line to divide headers from body, see RFC5322 */
    codigo,
    "\r\n",
    "Use this code to acess your file\r\n",
    "warming: be quick, you have less then 30 secs left to input your code\r\n",
    NULL
  };


  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;

  upload_ctx.lines_read = 0;
  *upload_ctx.payload = *payload_to_send;

  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */

    //nota: isto corresponde a um email da google
    //portanto devo criar um novo e colocar aqui as credenciais associadas
    curl_easy_setopt(curl, CURLOPT_USERNAME, "tp3ssiezenad");
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "palavrapasse2");

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
    char pathTocertificate[FILENAME_MAX];
    strcpy(pathTocertificate,pathToFolder);
    strcat(pathTocertificate,"/cacert.pem");

    curl_easy_setopt(curl, CURLOPT_CAINFO, pathTocertificate);

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
    recipients = curl_slist_append(recipients, destinatario);
    recipients = curl_slist_append(recipients, cc);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */

    //nota: aqui é que ele coloca o payload
    //pelo que alterar a mensagem será alterar o payload_source
    //para algo diferente

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source_custom);
    
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

/*

int main(void){
  sendMailToSomeoneWithACode("99ezequiel90@gmail.com","amazingsecurecode");
  return 0;
}

*/
