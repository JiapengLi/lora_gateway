/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
	Minimum test program for the loragw_hal 'library'

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont and Jiapeng Li
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
	#define _XOPEN_SOURCE 600
#else
	#define _XOPEN_SOURCE 500
#endif

#include <stdint.h>		/* C99 types */
#include <stdbool.h>	/* bool type */
#include <stdio.h>		/* printf */
#include <string.h>		/* memset */
#include <signal.h>		/* sigaction */
#include <stdlib.h>

#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#if ((CFG_BAND_868 == 1) || ((CFG_BAND_FULL == 1) && (CFG_RADIO_1257 == 1)))
	#define	F_RX_0	868500000
	#define	F_RX_1	869500000
	#define	F_TX	869000000
#elif (CFG_BAND_780 == 1)
	#define	F_RX_0	780500000
	#define	F_RX_1	781500000
	#define	F_TX	780000000
#elif (CFG_BAND_915 == 1)
	#define	F_RX_0	914500000
	#define	F_RX_1	915500000
	#define	F_TX	915000000
#elif ((CFG_BAND_470 == 1) || ((CFG_BAND_FULL == 1) && (CFG_RADIO_1255 == 1)))
	#define	F_RX_0	471500000
	#define	F_RX_1	472500000
	#define	F_TX	472000000
#elif (CFG_BAND_433 == 1)
	#define	F_RX_0	433500000
	#define	F_RX_1	434500000
	#define	F_TX	434000000
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

static void sig_handler(int sigio);

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void sig_handler(int sigio) {
	if (sigio == SIGQUIT) {
		quit_sig = 1;;
	} else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
		exit_sig = 1;
	}
}

#define CHANNEL_NUM		8

int offset_tab[CHANNEL_NUM][3] = {
	{ 200000, 0, F_RX_0,},
	{      0, 0, F_RX_0,},
	{-200000, 0, F_RX_0,},
	{-400000, 0, F_RX_0,},
	{ 200000, 1, F_RX_1,},
	{      0, 1, F_RX_1,},
	{-200000, 1, F_RX_1,},
	{-400000, 1, F_RX_1,},
};

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

int main(int argc, char ** argv)
{
	struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
	
	struct lgw_conf_rxrf_s rfconf;
	struct lgw_conf_rxif_s ifconf;
	
	struct lgw_pkt_rx_s rxpkt[4]; /* array containing up to 4 inbound packets metadata */
	struct lgw_pkt_rx_s *p; /* pointer on a RX packet */
	
	int i, j;
	int nb_pkt;
	
	int channel_num = CHANNEL_NUM;

	switch(argc){
		case 1:
			channel_num = CHANNEL_NUM;
			break;
		case 2:
			channel_num = atoi(argv[1]);
			if(channel_num > 0 && channel_num < 9){

			}else{
				printf("\nUsage: test_loragw_rx ChannelNumber(uint)\n8 channels maximum, without parameter to select 8 channels\n\n");
				exit(-1);
			}
			break;
		default:
			printf("\nUsage: test_loragw_rx ChannelNumber(uint)\n8 channels maximum, without parameter to select 8 channels\n\n");
			exit(-1);
			break;
	}

	/* configure signal handling */
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sig_handler;
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	
	/* beginning of LoRa concentrator-specific code */
	printf("Beginning of test for loragw_hal.c\n");
	
	printf("*** Library version information ***\n%s\n\n", lgw_version_info());
	
	printf("F_RX0 = %d, F_RX1 = %d\n", F_RX_0, F_RX_1);

	printf("%d freqeuncy channels are selected\n", channel_num);
	for(i=0; i<channel_num; i++){
		printf("channel: %d, freq: %d\n", i, offset_tab[i][0] + offset_tab[i][2]);
	}

	/* set configuration for RF chains */
	memset(&rfconf, 0, sizeof(rfconf));

	/* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
	memset(&ifconf, 0, sizeof(ifconf));

	

	/* initialize all channels */
	for(i=0; i<channel_num; i++){
		rfconf.enable = true;
		rfconf.freq_hz = offset_tab[i][2];
		lgw_rxrf_setconf(offset_tab[i][1], rfconf);

		ifconf.enable = true;
		ifconf.rf_chain = offset_tab[i][1];
		ifconf.freq_hz = offset_tab[i][0];
		ifconf.datarate = DR_LORA_MULTI;
		lgw_rxif_setconf(i, ifconf); /* chain 0: LoRa 125kHz, all SF, on f0 - 0.3 MHz */
	}
	
	/* set configuration for LoRa 'stand alone' channel */
	memset(&ifconf, 0, sizeof(ifconf));
	ifconf.enable = true;
	ifconf.rf_chain = 0;
	ifconf.freq_hz = 0;
	ifconf.bandwidth = BW_250KHZ;
	ifconf.datarate = DR_LORA_SF10;
	lgw_rxif_setconf(8, ifconf); /* chain 8: LoRa 250kHz, SF10, on f0 MHz */
	
	/* set configuration for FSK channel */
	memset(&ifconf, 0, sizeof(ifconf));
	ifconf.enable = true;
	ifconf.rf_chain = 1;
	ifconf.freq_hz = 0;
	ifconf.bandwidth = BW_250KHZ;
	ifconf.datarate = 64000;
	lgw_rxif_setconf(9, ifconf); /* chain 9: FSK 64kbps, on f1 MHz */
	
	/* connect, configure and start the LoRa concentrator */
	i = lgw_start();
	if (i == LGW_HAL_SUCCESS) {
		printf("*** Concentrator started ***\n");
	} else {
		printf("*** Impossible to start concentrator ***\n");
		return -1;
	}
	
	/* once configured, dump content of registers to a file, for reference */
	// FILE * reg_dump = NULL;
	// reg_dump = fopen("reg_dump.log", "w");
	// if (reg_dump != NULL) {
		// lgw_reg_check(reg_dump);
		// fclose(reg_dump);
	// }
	
	while ((quit_sig != 1) && (exit_sig != 1)) {
		
		/* fetch N packets */
		nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
		
		if (nb_pkt == 0) {
			wait_ms(300);
		} else {
			/* display received packets */
			for(i=0; i < nb_pkt; ++i) {
				p = &rxpkt[i];
				printf("---\nRcv pkt #%d >>", i+1);
				printf("freq:%d\n", offset_tab[p->if_chain][2]+offset_tab[p->if_chain][0]);
				if (p->status == STAT_CRC_OK) {
					printf(" if_chain:%2d", p->if_chain);
					printf(" tstamp:%010u", p->count_us);
					printf(" size:%3u", p->size);
					switch (p-> modulation) {
						case MOD_LORA: printf(" LoRa"); break;
						case MOD_FSK: printf(" FSK"); break;
						default: printf(" modulation?");
					}
					switch (p->datarate) {
						case DR_LORA_SF7: printf(" SF7"); break;
						case DR_LORA_SF8: printf(" SF8"); break;
						case DR_LORA_SF9: printf(" SF9"); break;
						case DR_LORA_SF10: printf(" SF10"); break;
						case DR_LORA_SF11: printf(" SF11"); break;
						case DR_LORA_SF12: printf(" SF12"); break;
						default: printf(" datarate?");
					}
					switch (p->coderate) {
						case CR_LORA_4_5: printf(" CR1(4/5)"); break;
						case CR_LORA_4_6: printf(" CR2(2/3)"); break;
						case CR_LORA_4_7: printf(" CR3(4/7)"); break;
						case CR_LORA_4_8: printf(" CR4(1/2)"); break;
						default: printf(" coderate?");
					}
					printf("\n");
					printf(" RSSI:%+6.1f SNR:%+5.1f (min:%+5.1f, max:%+5.1f) payload:\n", p->rssi, p->snr, p->snr_min, p->snr_max);
					
					for (j = 0; j < p->size; ++j) {
						printf(" %02X", p->payload[j]);
					}
					printf(" #\n");
				} else if (p->status == STAT_CRC_BAD) {
					printf(" if_chain:%2d", p->if_chain);
					printf(" tstamp:%010u", p->count_us);
					printf(" size:%3u\n", p->size);
					printf(" CRC error, damaged packet\n\n");
				} else if (p->status == STAT_NO_CRC){
					printf(" if_chain:%2d", p->if_chain);
					printf(" tstamp:%010u", p->count_us);
					printf(" size:%3u\n", p->size);
					printf(" no CRC\n\n");
				} else {
					printf(" if_chain:%2d", p->if_chain);
					printf(" tstamp:%010u", p->count_us);
					printf(" size:%3u\n", p->size);
					printf(" invalid status ?!?\n\n");
				}
			}
		}
	}
	
	if (exit_sig == 1) {
		/* clean up before leaving */
		lgw_stop();
	}
	
	printf("\nEnd of test for loragw_hal.c\n");
	return 0;
}

/* --- EOF ------------------------------------------------------------------ */
