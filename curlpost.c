#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
  CURL *curl;
  CURLcode res;

  /* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);

  /* get a curl handle */
  curl = curl_easy_init();
  if (curl)
  {

    //set POST URL here
    curl_easy_setopt(curl, CURLOPT_URL, "localhost:1234/play");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

    //perform the POST
    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return 0;
}
