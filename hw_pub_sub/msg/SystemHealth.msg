# System health monitoring
std_msgs/Header header
string node_name
float64 cpu_usage_percent
float64 memory_usage_percent
float64 network_latency_ms
int32 message_count
float64 message_rate_hz
bool is_healthy
string[] warnings
string[] errors
float64 uptime_seconds