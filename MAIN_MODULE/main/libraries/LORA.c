
#include <stdio.h>
#include "sdkconfig.h"
#include "LoRa.h"

const char *LORA_TAG = "LORA LIBRARY";
#define ESP_INTR_FLAG_DEFAULT 0

// Definition internal variables
spi_device_handle_t 	_spi;
int 					_packetIndex = 0;
int 					_loraImplicitHeaderMode = 0;
bool					_dataReceived = false;
long					_frequency;

// Local Functions definition
static void writeRegister( uint8_t reg, uint8_t data );
static uint8_t loraReadRegister( uint8_t reg );
static void delay( int delay );
static void initialize( int power );
static void initializeSPI( int mosi, int miso, int clk, int cs );
static void initializeReset( int reset );
static void initializeDIO( int dio );
static void IRAM_ATTR gpio_isr_handler(void* arg);


/*
 *  PUBLIC FUNCTIONS
 ****************************************************************************************
 */


void loraInit( int mosi, int miso, int clk, int cs, int reset, int dio, int power ) {
	initializeSPI( mosi, miso, clk, cs );
	initializeReset( reset );
	initializeDIO( dio );
	initialize( power );

}

void loraIdle() {
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void loraSleep() {
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void loraSetFrequency(long frequency) {
	_frequency = frequency;

	uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
	writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
	writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
	writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void loraSetSpreadingFactor(int sf) {
	if (sf < 6)
		sf = 6;
	else if (sf > 12)
		sf = 12;

	if (sf == 6) 	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
	}
	else 	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
	}

	int val = (loraReadRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0);
	writeRegister(REG_MODEM_CONFIG_2, val );
}

void loraSetSignalBandwidth(long sbw) {
	int bw;

	if (sbw <= 7.8E3) { 
		bw = 0; 
	} else if (sbw <= 10.4E3) { 
		bw = 1; 
	} else if (sbw <= 15.6E3) { 
		bw = 2; 
	} else if (sbw <= 20.8E3) { 
		bw = 3; 
	} else if (sbw <= 31.25E3) { 
		bw = 4; 
	} else if (sbw <= 41.7E3) { 
		bw = 5; 
	} else if (sbw <= 62.5E3) { 
		bw = 6; 
	} else if (sbw <= 125E3) { 
		bw = 7; 
	} else if (sbw <= 250E3) { 
		bw = 8; 
	} else /*if (sbw <= 250E3)*/ { 
		bw = 9; 
	}
	writeRegister(REG_MODEM_CONFIG_1,(loraReadRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
}

void loraSetSyncWord(int sw) {
	writeRegister(REG_SYNC_WORD, sw);
}

void loraSetCRC( bool crc ) {
	if ( crc )
		writeRegister(REG_MODEM_CONFIG_2, loraReadRegister(REG_MODEM_CONFIG_2) | 0x04);
	else
		writeRegister(REG_MODEM_CONFIG_2, loraReadRegister(REG_MODEM_CONFIG_2) & 0xfb);
}


// Enables overload current protection (OCP) for PA:
// 		0x20 OCP enabled
//		0x00 OCP enabled
//
// Trimming of OCP current:
//		I max = 45+5*OcpTrim [mA] if OcpTrim <= 15 (120 mA) /
//		I max = -30+10*OcpTrim [mA] if 15 < OcpTrim <= 27 (130 to
//				240 mA)
// 		I max = 240mA for higher settings
//		Default I max = 100mA

void loraSetOCP(uint8_t mA) {
	uint8_t ocpTrim = 27;

	if (mA <= 120) ocpTrim = (mA - 45) / 5;
	else if (mA <=240) ocpTrim = (mA + 30) / 10;

	writeRegister(REG_LR_OCP, 0x20 | (0x1F & ocpTrim));
}


void loraSetTxPower(int8_t level, int8_t outputPin) {

	// If using the RFO pin for output then the output power
	// is restricted between 0 and 14 dBm

	if (PA_OUTPUT_RFO_PIN == outputPin) {
		if (level < 0) level = 0;
    	else if (level > 14) level = 14;

		writeRegister(REG_PA_CONFIG, 0x70 | level);

	}

	// Otherwise we are using the PA boost pin for output. In this
	// case if the output power is greater than 17 dBm then we need to
	// also enable high output on the PA DAC

	else {
		int paDAC = 0x84;

		if (level > 17) {
			if (level > 20) level = 20;

			// subtract 3 from level, so 18 - 20 maps to 15 - 17
			level -= 3;

			// High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
			paDAC = 0x87;
			loraSetOCP(140);
		}
		else {
			if (level < 2)
				level = 2;
			loraSetOCP(100);
		}

		int paConfig = PA_BOOST | (level - 2);

		printf( "RegPAConfig: [%02x]\n", paConfig);
		printf( "RegPADAC: [%02x]\n", paDAC);

		writeRegister(REG_PA_DAC, paDAC);
		writeRegister(REG_PA_CONFIG, paConfig );
	}
}


void loraExplicitHeaderMode() {
  _loraImplicitHeaderMode = 0;
  writeRegister(REG_MODEM_CONFIG_1, loraReadRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

void loraImplicitHeaderMode() {
  _loraImplicitHeaderMode = 1;
  writeRegister(REG_MODEM_CONFIG_1, loraReadRegister(REG_MODEM_CONFIG_1) | 0x01);
}

int loraBeginPacket(int implicitHeader) {
  // put in standby mode
  loraIdle();
  if (implicitHeader) {
    loraImplicitHeaderMode();
  } else {
    loraExplicitHeaderMode();
  }
  // reset FIFO address and paload length
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  writeRegister(REG_PAYLOAD_LENGTH, 0);
  return 1;
}

int loraEndPacket(bool async) {
  // put in TX mode
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

  if (async) {
    // grace time is required for the radio
    delay(1);
  } else {
	// wait for TX done
    while ((loraReadRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);
    // clear IRQ's
    writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
	//printf( "TX Done\n");
  }

  return 1;
}


size_t loraWrite(const uint8_t *buffer, size_t size) {
  int currentLength = loraReadRegister(REG_PAYLOAD_LENGTH);
  //printf( "Current: %d Size: %d\n" , currentLength, size );

  // check size
  if ((currentLength + size) > MAX_PKT_LENGTH) {
    size = MAX_PKT_LENGTH - currentLength;
  }

  //printf( "New Size: %d\n" , size );

  // write data
  for (size_t i = 0; i < size; i++) {
    writeRegister(REG_FIFO, buffer[i]);
  }
  // update length
  writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);
  return size;
}

void loraDumpRegisters() {
  for (int i = 0; i < 128; i++)
	  printf( "%02x: %02x\n", i, loraReadRegister(i) );
}

int loraAvailable() {
	//printf( "RX Nbr Bytes: [%d]\n", loraReadRegister(REG_RX_NB_BYTES) );
	return (loraReadRegister(REG_RX_NB_BYTES) - _packetIndex);
}

int loraRead() {
	if ( !loraAvailable() )
		return -1;

	_packetIndex++;
	return loraReadRegister(REG_FIFO);
}

void loraReceive(int size) {
	if (size > 0) {
		loraImplicitHeaderMode();
		writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
	} else
		loraExplicitHeaderMode();

	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

int loraGetPacketRssi() {
	int8_t snr=0;
    int8_t SnrValue = loraReadRegister( 0x19 );
    int16_t rssi = loraReadRegister(REG_PKT_RSSI_VALUE);

	if( SnrValue & 0x80 ) {		// The SNR sign bit is 1
		// Invert and divide by 4
		snr = ( ( ~SnrValue + 1 ) & 0xFF ) >> 2;
		snr = -snr;
	} else {
		// Divide by 4
		snr = ( SnrValue & 0xFF ) >> 2;
	}
    if(snr<0) {
    	rssi = rssi - (_frequency < 525E6 ? 164 : 157) + ( rssi >> 4 ) + snr;
    } else {
    	rssi = rssi - (_frequency < 525E6 ? 164 : 157) + ( rssi >> 4 );
    }

  return ( rssi );
}


int loraHandleDataReceived( char *msg ) {
	int irqFlags = loraReadRegister(REG_IRQ_FLAGS);
	writeRegister(REG_IRQ_FLAGS, irqFlags);

	if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
		int packetLength = _loraImplicitHeaderMode ? loraReadRegister(REG_PAYLOAD_LENGTH) : loraReadRegister(REG_RX_NB_BYTES);
		printf( "In handleDataReceived: len: %d fifo_addr: %d rx_addr: %d\n",
				packetLength,
				loraReadRegister(REG_FIFO_ADDR_PTR),
				loraReadRegister(REG_FIFO_RX_CURRENT_ADDR) );

		_packetIndex = 0;

		writeRegister(REG_FIFO_ADDR_PTR, loraReadRegister(REG_FIFO_RX_CURRENT_ADDR));
//		if (_onReceive) { _onReceive(packetLength); }

		for (int i = 0; i < packetLength; i++)
			*msg++ = loraRead();
		*msg = '\0';

		writeRegister(REG_FIFO_ADDR_PTR, 0);
		return packetLength;
	}

	return 0;
}

int loraParsePacket(int size) {
	int packetLength = 0;

	if (size > 0) {
		loraImplicitHeaderMode();
		writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
	} else
		loraExplicitHeaderMode();

	// Check the IRQ_RX_DONE interrupt. If we have one that means there
	// is data loraReady to loraRead. Clear the IRQ in any case.

	int irqFlags = loraReadRegister(REG_IRQ_FLAGS);
	writeRegister(REG_IRQ_FLAGS, irqFlags);

  	if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
  		_packetIndex = 0;

  		if (_loraImplicitHeaderMode)
  			packetLength = loraReadRegister(REG_PAYLOAD_LENGTH);
  		else
  			packetLength = loraReadRegister(REG_RX_NB_BYTES);

  		// set FIFO address to current RX address
  		writeRegister(REG_FIFO_ADDR_PTR, loraReadRegister(REG_FIFO_RX_CURRENT_ADDR));
  		// put in standby mode
  		loraIdle();
  	} else if (loraReadRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
    // not currently in RX mode
    // reset FIFO address
  		writeRegister(REG_FIFO_ADDR_PTR, 0);
  		writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
  	}

  	return packetLength;
}


void loraSetDataReceived( bool r ) { 
	_dataReceived = r; 
}

bool loraGetDataReceived() { 
	return _dataReceived; 
}




/*
 *  Local FUNCTIONS
 ****************************************************************************************
 */

static void initializeDIO( int dio ) {
    gpio_config_t io_conf;
    gpio_num_t pin = (gpio_num_t) dio;

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << pin );
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add( pin, gpio_isr_handler, (void*) pin );

}

static void initialize( int power ) {
	loraSleep();

    uint8_t version = loraReadRegister(REG_VERSION);
    printf( "Version: [%d]\n", version );
    loraSetFrequency( 433E6 );

    writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
    writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

    writeRegister(REG_LNA, loraReadRegister(REG_LNA) | 0x03);
    writeRegister(REG_MODEM_CONFIG_3, 0x04);

    loraSetTxPower(power, RF_PACONFIG_PASELECT_PABOOST);
    // loraSetTxPower(power, RF_PACONFIG_PASELECT_RFO);

    loraSetSpreadingFactor(12);
	loraSetSignalBandwidth(125E3);
	loraSetSyncWord(0x34);

	loraSetCRC( false );
	loraSetCRC( true );

	loraIdle();

}

static void delay( int msec ) {
    vTaskDelay( msec / portTICK_PERIOD_MS);
}


static void writeRegister( uint8_t reg, uint8_t data ) {
    //printf("Writing Register [%02x]=[%02x]\n", reg, data);
	reg = reg | 0x80;

	spi_transaction_t transaction;
	memset( &transaction, 0, sizeof(spi_transaction_t) );

	transaction.length = 8;
	transaction.rxlength = 0;
	transaction.addr = reg;
	transaction.flags = SPI_TRANS_USE_TXDATA;

	memcpy(transaction.tx_data, &data, 1);

	esp_err_t err = spi_device_polling_transmit(_spi, &transaction);
}

static uint8_t loraReadRegister( uint8_t reg ) {
	uint8_t result;

	spi_transaction_t transaction;
	memset( &transaction, 0, sizeof(spi_transaction_t) );

	transaction.length = 0;
	transaction.rxlength = 8;
	transaction.addr = reg & 0x7f;
	transaction.flags = SPI_TRANS_USE_RXDATA;

	esp_err_t err = spi_device_polling_transmit( _spi, &transaction);

	memcpy(&result, transaction.rx_data, 1);
//    printf("loraReading Register [%02x]=[%02x]\n", reg, result);

	return result;
}



static void initializeSPI( int mosi, int miso, int clk, int cs ) {
    esp_err_t ret;

    spi_bus_config_t buscfg;
	memset( &buscfg, 0, sizeof(spi_bus_config_t) );

    buscfg.mosi_io_num = mosi;
    buscfg.miso_io_num = miso;
	buscfg.sclk_io_num = clk;
	buscfg.quadwp_io_num = -1;
	buscfg.quadhd_io_num = -1;
	buscfg.max_transfer_sz = 0;
	buscfg.flags = 0;
	buscfg.intr_flags = 0;


    // Started working after reduce clock speed to 8MHz but then when I changed
    // back to 10 Mhz it continued working. Not sure whats going on

    spi_device_interface_config_t devcfg;
	memset( &devcfg, 0, sizeof(spi_device_interface_config_t) );

   	devcfg.address_bits = 8;
    devcfg.mode=0;
	devcfg.clock_speed_hz=200000;
	devcfg.spics_io_num=cs;
	devcfg.flags = SPI_DEVICE_HALFDUPLEX;
	devcfg.queue_size = 1;


    ret=spi_bus_initialize(SPI2_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    printf("Bus Init: %d\n", ret);

    if ( ret > 0 )
    	return;

    ret=spi_bus_add_device(SPI2_HOST, &devcfg, &_spi);
    ESP_ERROR_CHECK(ret);
    printf("Add device: %d\n", ret);


}

static void initializeReset( int reset ) {
	gpio_num_t r = (gpio_num_t) reset;

    gpio_pad_select_gpio( r );
    gpio_set_direction( r, GPIO_MODE_OUTPUT);

    gpio_set_level(r, 0);
    delay(50);
    gpio_set_level(r, 1);
    delay(50);

}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    loraSetDataReceived(true);
}


