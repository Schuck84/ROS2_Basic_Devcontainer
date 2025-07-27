# Sensor data message with timestamp and various sensor readings
std_msgs/Header header
string sensor_name
float64 temperature
float64 humidity
float64 pressure
float64 x_acceleration
float64 y_acceleration
float64 z_acceleration
float64 x_gyroscope
float64 y_gyroscope
float64 z_gyroscope
float64 battery_level
bool is_calibrated
string status_message