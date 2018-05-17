#include <iostream>
#include <iomanip>

#include "include/rpi_bme280.h"
#include "Stream.hpp"

#define _DEVICE_A_

#ifdef _DEVICE_A_

#define DEVICE_ID 'a'
#define EXT_DEVICE_ID 'b'

#define TEMP_STREAM_ID "temperature_a"
#define HUM_STREAM_ID "humidity_a"
#define PRES_STREAM_ID "pressure_a"

#define EXT_TEMP_STREAM_ID "temperature_b"
#define EXT_HUM_STREAM_ID "humidity_b"
#define EXT_PRES_STREAM_ID "pressure_b"

#endif
#ifdef _DEVICE_B_

#define DEVICE_ID 'b'
#define EXT_DEVICE_ID 'a'

#define TEMP_STREAM_ID "temperature_b"
#define HUM_STREAM_ID "humidity_b"
#define PRES_STREAM_ID "pressure_b"

#define EXT_TEMP_STREAM_ID "temperature_a"
#define EXT_HUM_STREAM_ID "humidity_a"
#define EXT_PRES_STREAM_ID "pressure_a"

#endif

// Make use of the glasgow_ustream namespace.
using namespace glasgow_ustream;

//########### SERIALIZATION ############
auto double_to_bytes = [] (double val) {
	void *data = malloc(sizeof(double));
	*((double *) data) = val;
	return std::pair<uint32_t, void *>(sizeof(double), data);
};

optional<double> (*byte_array_to_double) (std::pair<uint32_t, void*>) = [] (std::pair<uint32_t, void*> data) {
    return optional<double>(*((double*) data.second));
};
//######################################

//######## SERIAL CONSOLE (TTY) PRINTERS ##########
auto print_tty_temperature_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Local temperature " << std::setprecision(5) << val << " °C" << std::endl;
};
auto print_tty_humidity_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Local humidity " << std::setprecision(5) << val << " %" << std::endl;
};
auto print_tty_pressure_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Local pressure " << std::setprecision(5) << val << " hPa" << std::endl;
};
//######################################

//######## THRESHOLD PRINTERS ##########
auto print_temperature_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Temperature differs by " << std::setprecision(5) << val << " °C" << std::endl;
};
auto print_humidity_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Humidity differs by " << std::setprecision(5) << val << " %" << std::endl;
};
auto print_pressure_sink = [](double val) {
	std::cout << TEMP_STREAM_ID << " Pressure differs by " << std::setprecision(5) << val << " hPa" << std::endl;
};
//######################################


//######## DIFFERENCE LAMBDAS ##########
double (*humidity_diff_map) (double) = [] (double val) {
	double diff = val - compensated_humidity_double;
	return (diff < 0) ? diff * -1 : diff;
};

double (*temperature_diff_map) (double) = [] (double val) {
	double diff = val - compensated_temperature_double;
	return (diff < 0) ? diff * -1 : diff;
};

double (*pressure_diff_map) (double) = [] (double val) {
	double diff = val - compensated_pressure_double;
	return (diff < 0) ? diff * -1 : diff;
};
//#######################################

// This class is used to poll the current sensor readings.
class BME280_Value_Pollable : public Pollable<double> {

	double *ptr_to_value;

public:

	explicit BME280_Value_Pollable(double *ptr) : ptr_to_value(ptr) {}

	double getData(PolledSource<double> *) override {
		return *ptr_to_value;
	}

	virtual ~BME280_Value_Pollable(){}
};


int main(void) {
	//connect_wifi();

	//i2c_master_init();

	// Use a priority of 10 to prevent preemption. (default Glasgow MicroStream thread has priority 5)
	//xTaskCreate(&task_bme280_normal_mode, "bme280_normal_mode",  2048, NULL, 10, NULL);
	open_bme280();
	std::thread t1(thread_read_bme280);

	BME280_Value_Pollable *humidity_pollable = new BME280_Value_Pollable(&compensated_humidity_double);
	BME280_Value_Pollable *temperature_pollable = new BME280_Value_Pollable(&compensated_temperature_double);
	BME280_Value_Pollable *pressure_pollable = new BME280_Value_Pollable(&compensated_pressure_double);

	// Create a topology.  The default constructor uses PeerDiscovery with a broadcast period of 5 seconds.
	Topology *topology = new Topology();

	// Poll each sensor reading every second
	Stream<double> *humidity_stream = topology->addPolledSource(std::chrono::seconds(1), humidity_pollable);
	Stream<double> *temperature_stream =topology->addPolledSource(std::chrono::seconds(1), temperature_pollable);
	Stream<double> *pressure_stream =topology->addPolledSource(std::chrono::seconds(1), pressure_pollable);

	temperature_stream->sink(print_tty_temperature_sink);
	humidity_stream->sink(print_tty_humidity_sink);
	pressure_stream->sink(print_tty_pressure_sink);

	// Let everyone in the network know our sensor readings
	humidity_stream->networkSink(topology, HUM_STREAM_ID, double_to_bytes);
	temperature_stream->networkSink(topology, TEMP_STREAM_ID, double_to_bytes);
	pressure_stream->networkSink(topology, PRES_STREAM_ID, double_to_bytes);

	// Get the sensor readings from the other device on the network.
	NetworkSource<double> *ext_humidity_stream = topology->addNetworkSource(EXT_HUM_STREAM_ID, byte_array_to_double).value();
	NetworkSource<double> *ext_temperature_stream = topology->addNetworkSource(EXT_TEMP_STREAM_ID, byte_array_to_double).value();
	NetworkSource<double> *ext_pressure_stream = topology->addNetworkSource(EXT_PRES_STREAM_ID, byte_array_to_double).value();

	// Calculate the difference between the other device's readings, and ours.
	MapStream<double,double> *humidity_diff_stream = ext_humidity_stream->map(humidity_diff_map);
	MapStream<double,double> *temperature_diff_stream = ext_temperature_stream->map(temperature_diff_map);
	MapStream<double,double> *pressure_diff_stream = ext_pressure_stream->map(pressure_diff_map);

	// Print the readings if they differ by certain thresholds.
	temperature_diff_stream->filter([] (double diff) {
		return diff > 1; // 1 degree C
	})->sink(print_temperature_sink);

	humidity_diff_stream->filter([] (double diff) {
		return diff > 3; // 3%
	})->sink(print_humidity_sink);

	pressure_diff_stream->filter([] (double diff) {
		return diff > 4; // hPa
	})->sink(print_pressure_sink);


	topology->run();

  return 0;
}
