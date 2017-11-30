/*********************************************************************************
* imu.c
*
* This program uses the beaglebone blue's accelerometer and gyroscope, which are
* located in the IMU, to detect user gestures. The gestures express the user's 
* wish to adjust the volume or move to the prev/next song. Once a gesture is 
* detected, the music control API to sends out a POST message to the music server.
*********************************************************************************/

#include <rc_usefulincludes.h>
#include <roboticscape.h>
#include <sys/time.h>
#include <stdio.h>
#include <curl/curl.h>
#include <alsa/asoundlib.h>

#define VOLUME_THRESHOLD 5.0
#define TOGGLE_SONG_THRESHOLD 100.0

void curl(char *endpoint);
void setVolume(long newVolume);

long curVolume = 50;

int main()
{
	rc_imu_data_t data; //struct to hold new data

	// initialize robotics cape hardware
	if (rc_initialize())
	{
		fprintf(stderr, "ERROR: failed to run rc_initialize(), are you root?\n");
		return -1;
	}

	// use defaults for now, except also enable magnetometer.
	rc_imu_config_t conf = rc_default_imu_config();
	conf.enable_magnetometer = 1;

	// initialize imu unit on the robotics cape
	if (rc_initialize_imu(&data, conf))
	{
		fprintf(stderr, "rc_initialize_imu_failed\n");
		return -1;
	}

	// initialize time struct for song toggle timeout
	struct timespec time_spec;
	clock_gettime(CLOCK_REALTIME, &time_spec);
	int newtime = time_spec.tv_sec++;
	int time = newtime;

	curl("play");
	while (rc_get_state() != EXITING)
	{
		// read data from accelerometer for volume control
		if (rc_read_accel_data(&data) < 0)
		{
			printf("read accel data failed\n");
		}
		float x = data.accel[0];
		// DEBUG: print accelerometer output
		// printf("%6.2f %6.2f %6.2f \n",data.accel[0],data.accel[1],data.accel[2]);

		// adjust volume if the x-axis acceleration exceeds the threshold
		if (x > VOLUME_THRESHOLD)
		{
			printf("volume +++++++++++++++++++++++++++\n");
			curVolume = curVolume + 2;
			setVolume(curVolume);
		}
		else if (x < -1.0 * VOLUME_THRESHOLD)
		{
			printf("volume ---------------------------\n");
			curVolume = curVolume - 2;
			setVolume(curVolume);
		}

		// read data from gyroscope to toggle song
		if (rc_read_gyro_data(&data) < 0)
		{
			printf("read gyro data failed\n");
		}
		// DEBUG: print gyroscope output
		// printf("%6.1f %6.1f %6.1f |\n",	data.gyro[0],data.gyro[1],data.gyro[2]);

		// Check for z-axis rotation if timeout is not enabled
		clock_gettime(CLOCK_REALTIME, &time_spec);
		newtime = time_spec.tv_sec;
		if (newtime >= time)
		{
			// toggle song if the z-axis rotation exceeds the threshold
			float z = data.gyro[2];
			if (z < -1.0 * TOGGLE_SONG_THRESHOLD)
			{
				printf("next song >>>>>>>>>>>>>>>\n");
				curl("next");
				time = newtime + 2;
			}
			else if (z > TOGGLE_SONG_THRESHOLD)
			{
				printf("prev song <<<<<<<<<<<<<<<\n");
				curl("previous");
				time = newtime + 2;
			}
		}
		rc_usleep(200000);
	}

	// cleanup imu and robotics cape resources before exiting
	rc_power_off_imu();
	rc_cleanup();
	return 0;
}

/*
 * Helper Method to perform POSTs to the music control API
 */
void curl(char *endpoint)
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
		char fulladdr[30];
		char *addr = "localhost:1234/";

		strcpy(fulladdr, addr);
		strcat(fulladdr, endpoint);

		curl_easy_setopt(curl, CURLOPT_URL, &fulladdr);

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
}

void setVolume(long newVolume)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, newVolume * max / 100);

	snd_mixer_close(handle);
}