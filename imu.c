/*******************************************************************************
* rc_test_imu.c
*
* This serves as an example of how to read the IMU with direct reads to the
* sensor registers. To use the DMP or interrupt-driven timing see test_dmp.c
*******************************************************************************/

#include <rc_usefulincludes.h>
#include <roboticscape.h>
#include <sys/time.h>
#include <stdio.h>
#include <curl/curl.h>
#include <alsa/asoundlib.h>

// possible modes, user selected with command line arguments
typedef enum m_mode_t {
	RAD,
	DEG,
	RAW
} m_mode_t;

void curl(char *endpoint);
void setVolume(long newVolume);

long curVolume = 50;

int main(int argc, char *argv[])
{
	rc_imu_data_t data; //struct to hold new data
	int c;
	m_mode_t mode = DEG; // default to radian mode.

	// initialize hardware first
	if (rc_initialize())
	{
		fprintf(stderr, "ERROR: failed to run rc_initialize(), are you root?\n");
		return -1;
	}

	// use defaults for now, except also enable magnetometer.
	rc_imu_config_t conf = rc_default_imu_config();
	conf.enable_magnetometer = 1;

	if (rc_initialize_imu(&data, conf))
	{
		fprintf(stderr, "rc_initialize_imu_failed\n");
		return -1;
	}

	// print a header
	printf("\ntry 'test_imu -h' to see other options\n\n");
	switch (mode)
	{
	case RAD:
		printf("   Accel XYZ(m/s^2)  |");
		printf("   Gyro XYZ (rad/s)  |");
		break;
	case DEG:
		printf("   Gyro XYZ (deg/s)  ");
		break;
	case RAW:
		printf("  Accel XYZ(raw adc) |");
		printf("  Gyro XYZ (raw adc) |");
		break;
	default:
		printf("ERROR: invalid mode\n");
		return -1;
	}
	printf("\n");

	if (rc_read_accel_data(&data) < 0)
	{
		printf("read gyro data failed\n");
	}

	struct timespec time_spec;
	clock_gettime(CLOCK_REALTIME, &time_spec);
	int newtime = time_spec.tv_sec++;
	int time = newtime;

	float origx = data.accel[0];
	float origy = data.accel[1];
	float origz = data.accel[2];

	float fx = 0;
	float fy = 0;
	float fz = 0;

	//printf("original values: %6.1f %6.1f %6.1f |\n", origx, origy, origz);
	//now just wait, print_data will run

	curl("play");
	while (rc_get_state() != EXITING)
	{
		printf("\r");

		//accelerometer stuff
		// print accel
		if (rc_read_accel_data(&data) < 0)
		{
			printf("read accel data failed\n");
		}
		float x = data.accel[0];
		float z = data.accel[1];
		float y = data.accel[2];
		//printf("%6.2f %6.2f %6.2f \n",data.accel[0],data.accel[1],data.accel[2]);

		if (x > 5.0)
		{
			printf("volume +++++++++++++++++++++++++++\n");
			curVolume = curVolume + 2;
			setVolume(curVolume);
		}
		else if (x < -5.0)
		{
			printf("volume ---------------------------\n");
			curVolume = curVolume - 2;
			setVolume(curVolume);
		}

		// print gyro data
		if (rc_read_gyro_data(&data) < 0)
		{
			printf("read gyro data failed\n");
		}
		float newY = data.gyro[1];

		//printf("%6.1f %6.1f %6.1f |\n",	data.gyro[0],\
						data.gyro[1],\
						data.gyro[2]);
		//printf("gyro diff: %f\n", newY - origy);
		clock_gettime(CLOCK_REALTIME, &time_spec);
		newtime = time_spec.tv_sec;
		if (newtime >= time)
		{

			if (data.gyro[2] < -100.0)
			{
				printf("next song >>>>>>>>>>>>>>>\n");
				curl("next");
				time = newtime + 2;
			}
			else if (data.gyro[2] > 100.0)
			{
				printf("prev song <<<<<<<<<<<<<<<\n");
				curl("previous");
				time = newtime + 2;
			}
		}
		rc_usleep(200000);
		//fflush(stdout);
	}

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