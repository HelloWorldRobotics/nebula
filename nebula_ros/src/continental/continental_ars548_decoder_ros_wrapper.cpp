// Copyright 2024 Tier IV, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <nebula_ros/continental/continental_ars548_decoder_ros_wrapper.hpp>

#include <pcl_conversions/pcl_conversions.h>

namespace nebula
{
namespace ros
{
ContinentalARS548DriverRosWrapper::ContinentalARS548DriverRosWrapper(
  const rclcpp::NodeOptions & options)
: rclcpp::Node("continental_ars548_driver_ros_wrapper", options), hw_interface_()
{
  drivers::continental_ars548::ContinentalARS548SensorConfiguration sensor_configuration;

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  hw_interface_.SetLogger(std::make_shared<rclcpp::Logger>(this->get_logger()));

  wrapper_status_ = GetParameters(sensor_configuration);
  if (Status::OK != wrapper_status_) {
    RCLCPP_ERROR_STREAM(this->get_logger(), this->get_name() << " Error:" << wrapper_status_);
    return;
  }
  RCLCPP_INFO_STREAM(this->get_logger(), this->get_name() << ". Starting...");

  sensor_cfg_ptr_ =
    std::make_shared<drivers::continental_ars548::ContinentalARS548SensorConfiguration>(
      sensor_configuration);

  wrapper_status_ = InitializeDriver(
    std::const_pointer_cast<drivers::continental_ars548::ContinentalARS548SensorConfiguration>(
      sensor_cfg_ptr_));

  RCLCPP_INFO_STREAM(this->get_logger(), this->get_name() << "Wrapper=" << wrapper_status_);
  packets_sub_ = create_subscription<nebula_msgs::msg::NebulaPackets>(
    "nebula_packets", rclcpp::SensorDataQoS(),
    std::bind(
      &ContinentalARS548DriverRosWrapper::ReceivePacketsMsgCallback, this, std::placeholders::_1));

  detection_list_pub_ =
    this->create_publisher<continental_msgs::msg::ContinentalArs548DetectionList>(
      "continental_detections", rclcpp::SensorDataQoS());
  object_list_pub_ = this->create_publisher<continental_msgs::msg::ContinentalArs548ObjectList>(
    "continental_objects", rclcpp::SensorDataQoS());

  detection_pointcloud_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
    "detection_points", rclcpp::SensorDataQoS());
  object_pointcloud_pub_ =
    this->create_publisher<sensor_msgs::msg::PointCloud2>("object_points", rclcpp::SensorDataQoS());

  scan_raw_pub_ =
    this->create_publisher<radar_msgs::msg::RadarScan>("scan_raw", rclcpp::SensorDataQoS());

  objects_raw_pub_ =
    this->create_publisher<radar_msgs::msg::RadarTracks>("objects_raw", rclcpp::SensorDataQoS());

  objects_markers_pub_ =
    this->create_publisher<visualization_msgs::msg::MarkerArray>("marker_array", 10);
}

void ContinentalARS548DriverRosWrapper::ReceivePacketsMsgCallback(
  const nebula_msgs::msg::NebulaPackets::SharedPtr scan_msg)
{
  decoder_ptr_->ProcessPackets(*scan_msg);
}

Status ContinentalARS548DriverRosWrapper::InitializeDriver(
  std::shared_ptr<drivers::SensorConfigurationBase> sensor_configuration)
{
  decoder_ptr_ = std::make_shared<drivers::continental_ars548::ContinentalARS548Decoder>(
    std::static_pointer_cast<drivers::continental_ars548::ContinentalARS548SensorConfiguration>(
      sensor_configuration));

  decoder_ptr_->RegisterDetectionListCallback(std::bind(
    &ContinentalARS548DriverRosWrapper::DetectionListCallback, this, std::placeholders::_1));
  decoder_ptr_->RegisterObjectListCallback(
    std::bind(&ContinentalARS548DriverRosWrapper::ObjectListCallback, this, std::placeholders::_1));

  return Status::OK;
}

Status ContinentalARS548DriverRosWrapper::GetStatus()
{
  return wrapper_status_;
}

Status ContinentalARS548DriverRosWrapper::GetParameters(
  drivers::continental_ars548::ContinentalARS548SensorConfiguration & sensor_configuration)
{
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<std::string>("host_ip", descriptor);
    sensor_configuration.host_ip = this->get_parameter("host_ip").as_string();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<std::string>("sensor_ip", descriptor);
    sensor_configuration.sensor_ip = this->get_parameter("sensor_ip").as_string();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("data_port", descriptor);
    sensor_configuration.data_port = this->get_parameter("data_port").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<std::string>("sensor_model");
    sensor_configuration.sensor_model =
      nebula::drivers::SensorModelFromString(this->get_parameter("sensor_model").as_string());
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
    descriptor.read_only = false;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<std::string>("frame_id", descriptor);
    sensor_configuration.frame_id = this->get_parameter("frame_id").as_string();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
    descriptor.read_only = false;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<std::string>("base_frame", descriptor);
    sensor_configuration.base_frame = this->get_parameter("base_frame").as_string();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_BOOL;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<bool>("use_sensor_time", descriptor);
    sensor_configuration.use_sensor_time = this->get_parameter("use_sensor_time").as_bool();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_plug_orientation", descriptor);
    sensor_configuration.new_plug_orientation =
      this->get_parameter("new_plug_orientation").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<double>("new_vehicle_length", descriptor);
    sensor_configuration.new_vehicle_length =
      static_cast<float>(this->get_parameter("new_vehicle_length").as_double());
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<double>("new_vehicle_width", descriptor);
    sensor_configuration.new_vehicle_width =
      static_cast<float>(this->get_parameter("new_vehicle_width").as_double());
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<double>("new_vehicle_height", descriptor);
    sensor_configuration.new_vehicle_height =
      static_cast<float>(this->get_parameter("new_vehicle_height").as_double());
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<double>("new_vehicle_wheelbase", descriptor);
    sensor_configuration.new_vehicle_wheelbase =
      static_cast<float>(this->get_parameter("new_vehicle_wheelbase").as_double());
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_maximum_distance", descriptor);
    sensor_configuration.new_radar_maximum_distance =
      this->get_parameter("new_radar_maximum_distance").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_frequency_slot", descriptor);
    sensor_configuration.new_radar_frequency_slot =
      this->get_parameter("new_radar_frequency_slot").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_cycle_time", descriptor);
    sensor_configuration.new_radar_cycle_time =
      this->get_parameter("new_radar_cycle_time").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_time_slot", descriptor);
    sensor_configuration.new_radar_time_slot = this->get_parameter("new_radar_time_slot").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_country_code", descriptor);
    sensor_configuration.new_radar_country_code =
      this->get_parameter("new_radar_country_code").as_int();
  }
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.type = rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER;
    descriptor.read_only = true;
    descriptor.dynamic_typing = false;
    descriptor.additional_constraints = "";
    this->declare_parameter<uint16_t>("new_radar_powersave_standstill", descriptor);
    sensor_configuration.new_radar_powersave_standstill =
      this->get_parameter("new_radar_powersave_standstill").as_int();
  }

  if (sensor_configuration.sensor_model == nebula::drivers::SensorModel::UNKNOWN) {
    return Status::INVALID_SENSOR_MODEL;
  }

  std::shared_ptr<drivers::SensorConfigurationBase> sensor_cfg_ptr =
    std::make_shared<drivers::continental_ars548::ContinentalARS548SensorConfiguration>(
      sensor_configuration);

  hw_interface_.SetSensorConfiguration(
    std::static_pointer_cast<drivers::SensorConfigurationBase>(sensor_cfg_ptr));

  RCLCPP_INFO_STREAM(this->get_logger(), "SensorConfig:" << sensor_configuration);
  return Status::OK;
}

void ContinentalARS548DriverRosWrapper::DetectionListCallback(
  std::unique_ptr<continental_msgs::msg::ContinentalArs548DetectionList> msg)
{
  if (
    detection_pointcloud_pub_->get_subscription_count() > 0 ||
    detection_pointcloud_pub_->get_intra_process_subscription_count() > 0) {
    const auto detection_pointcloud_ptr = ConvertToPointcloud(*msg);
    auto detection_pointcloud_msg_ptr = std::make_unique<sensor_msgs::msg::PointCloud2>();
    pcl::toROSMsg(*detection_pointcloud_ptr, *detection_pointcloud_msg_ptr);

    detection_pointcloud_msg_ptr->header = msg->header;
    detection_pointcloud_pub_->publish(std::move(detection_pointcloud_msg_ptr));
  }

  if (
    scan_raw_pub_->get_subscription_count() > 0 ||
    scan_raw_pub_->get_intra_process_subscription_count() > 0) {
    auto radar_scan_msg = ConvertToRadarScan(*msg);
    radar_scan_msg.header = msg->header;
    scan_raw_pub_->publish(std::move(radar_scan_msg));
  }

  if (
    detection_list_pub_->get_subscription_count() > 0 ||
    detection_list_pub_->get_intra_process_subscription_count() > 0) {
    detection_list_pub_->publish(std::move(msg));
  }
}

void ContinentalARS548DriverRosWrapper::ObjectListCallback(
  std::unique_ptr<continental_msgs::msg::ContinentalArs548ObjectList> msg)
{
  if (
    object_pointcloud_pub_->get_subscription_count() > 0 ||
    object_pointcloud_pub_->get_intra_process_subscription_count() > 0) {
    const auto object_pointcloud_ptr = ConvertToPointcloud(*msg);
    auto object_pointcloud_msg_ptr = std::make_unique<sensor_msgs::msg::PointCloud2>();
    pcl::toROSMsg(*object_pointcloud_ptr, *object_pointcloud_msg_ptr);

    object_pointcloud_msg_ptr->header = msg->header;
    object_pointcloud_pub_->publish(std::move(object_pointcloud_msg_ptr));
  }

  if (
    objects_raw_pub_->get_subscription_count() > 0 ||
    objects_raw_pub_->get_intra_process_subscription_count() > 0) {
    auto objects_raw_msg = ConvertToRadarTracks(*msg);
    objects_raw_msg.header = msg->header;
    objects_raw_pub_->publish(std::move(objects_raw_msg));
  }

  if (
    objects_markers_pub_->get_subscription_count() > 0 ||
    objects_markers_pub_->get_intra_process_subscription_count() > 0) {
    auto marker_array_msg = ConvertToMarkers(*msg);
    objects_markers_pub_->publish(std::move(marker_array_msg));
  }

  if (
    object_list_pub_->get_subscription_count() > 0 ||
    object_list_pub_->get_intra_process_subscription_count() > 0) {
    object_list_pub_->publish(std::move(msg));
  }
}

pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Detection>::Ptr
ContinentalARS548DriverRosWrapper::ConvertToPointcloud(
  const continental_msgs::msg::ContinentalArs548DetectionList & msg)
{
  pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Detection>::Ptr output_pointcloud(
    new pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Detection>);
  output_pointcloud->reserve(msg.detections.size());

  nebula::drivers::continental_ars548::PointARS548Detection point{};
  for (const auto & detection : msg.detections) {
    point.x =
      std::cos(detection.elevation_angle) * std::cos(detection.azimuth_angle) * detection.range;
    point.y =
      std::cos(detection.elevation_angle) * std::sin(detection.azimuth_angle) * detection.range;
    point.z = std::sin(detection.elevation_angle) * detection.range;

    point.azimuth = detection.azimuth_angle;
    point.azimuth_std = detection.azimuth_angle_std;
    point.elevation = detection.elevation_angle;
    point.elevation_std = detection.elevation_angle_std;
    point.range = detection.range;
    point.range_std = detection.range_std;
    point.range_rate = detection.range_rate;
    point.range_rate_std = detection.range_rate_std;
    point.rcs = detection.rcs;
    point.measurement_id = detection.measurement_id;
    point.positive_predictive_value = detection.positive_predictive_value;
    point.classification = detection.classification;
    point.multi_target_probability = detection.multi_target_probability;
    point.object_id = detection.object_id;
    point.ambiguity_flag = detection.ambiguity_flag;

    output_pointcloud->points.emplace_back(point);
  }

  output_pointcloud->height = 1;
  output_pointcloud->width = output_pointcloud->points.size();
  return output_pointcloud;
}

pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Object>::Ptr
ContinentalARS548DriverRosWrapper::ConvertToPointcloud(
  const continental_msgs::msg::ContinentalArs548ObjectList & msg)
{
  pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Object>::Ptr output_pointcloud(
    new pcl::PointCloud<nebula::drivers::continental_ars548::PointARS548Object>);
  output_pointcloud->reserve(msg.objects.size());

  nebula::drivers::continental_ars548::PointARS548Object point{};
  for (const auto & object : msg.objects) {
    const double half_length = 0.5 * object.shape_length_edge_mean;
    const double half_width = 0.5 * object.shape_width_edge_mean;
    const int reference_index = std::min<int>(object.position_reference, 8);
    const double & yaw = object.orientation;
    const double x = object.position.x +
                     std::cos(yaw) * half_length * reference_to_center_[reference_index][0] -
                     std::sin(yaw) * half_width * reference_to_center_[reference_index][1];
    const double y = object.position.y +
                     std::sin(yaw) * half_length * reference_to_center_[reference_index][0] +
                     std::cos(yaw) * half_width * reference_to_center_[reference_index][1];

    point.x = static_cast<float>(x);
    point.y = static_cast<float>(y);
    point.z = static_cast<float>(object.position.z);

    point.id = object.object_id;
    point.age = object.age;
    point.status_measurement = object.status_measurement;
    point.status_movement = object.status_movement;
    point.position_reference = object.position_reference;
    point.classification_car = object.classification_car;
    point.classification_truck = object.classification_truck;
    point.classification_motorcycle = object.classification_motorcycle;
    point.classification_bicycle = object.classification_bicycle;
    point.classification_pedestrian = object.classification_pedestrian;
    point.dynamics_abs_vel_x = static_cast<float>(object.absolute_velocity.x);
    point.dynamics_abs_vel_y = static_cast<float>(object.absolute_velocity.y);
    point.dynamics_rel_vel_x = static_cast<float>(object.relative_velocity.x);
    point.dynamics_rel_vel_y = static_cast<float>(object.relative_velocity.y);
    point.shape_length_edge_mean = object.shape_length_edge_mean;
    point.shape_width_edge_mean = object.shape_width_edge_mean;
    point.dynamics_orientation_rate_mean = object.orientation_rate_mean;

    output_pointcloud->points.emplace_back(point);
  }

  output_pointcloud->height = 1;
  output_pointcloud->width = output_pointcloud->points.size();
  return output_pointcloud;
}

radar_msgs::msg::RadarScan ContinentalARS548DriverRosWrapper::ConvertToRadarScan(
  const continental_msgs::msg::ContinentalArs548DetectionList & msg)
{
  radar_msgs::msg::RadarScan output_msg;
  output_msg.header = msg.header;
  output_msg.returns.reserve(msg.detections.size());

  radar_msgs::msg::RadarReturn return_msg;
  for (const auto & detection : msg.detections) {
    if (
      detection.invalid_azimuth || detection.invalid_distance || detection.invalid_elevation ||
      detection.invalid_range_rate) {
      continue;
    }

    return_msg.range = detection.range;
    return_msg.azimuth = detection.azimuth_angle;
    return_msg.elevation = detection.elevation_angle;
    return_msg.doppler_velocity = detection.range_rate;
    return_msg.amplitude = detection.rcs;
    output_msg.returns.emplace_back(return_msg);
  }

  return output_msg;
}

radar_msgs::msg::RadarTracks ContinentalARS548DriverRosWrapper::ConvertToRadarTracks(
  const continental_msgs::msg::ContinentalArs548ObjectList & msg)
{
  radar_msgs::msg::RadarTracks output_msg;
  output_msg.tracks.reserve(msg.objects.size());
  output_msg.header = msg.header;

  constexpr int16_t UNKNOWN_ID = 32000;
  constexpr int16_t CAR_ID = 32001;
  constexpr int16_t TRUCK_ID = 32002;
  constexpr int16_t MOTORCYCLE_ID = 32005;
  constexpr int16_t BICYCLE_ID = 32006;
  constexpr int16_t PEDESTRIAN_ID = 32007;
  constexpr float INVALID_COVARIANCE = 1e6;

  radar_msgs::msg::RadarTrack track_msg;
  for (const auto & object : msg.objects) {
    track_msg.uuid.uuid[0] = static_cast<uint8_t>(object.object_id & 0xff);
    track_msg.uuid.uuid[1] = static_cast<uint8_t>((object.object_id >> 8) & 0xff);
    track_msg.uuid.uuid[2] = static_cast<uint8_t>((object.object_id >> 16) & 0xff);
    track_msg.uuid.uuid[3] = static_cast<uint8_t>((object.object_id >> 24) & 0xff);

    const double half_length = 0.5 * object.shape_length_edge_mean;
    const double half_width = 0.5 * object.shape_width_edge_mean;
    const int reference_index = std::min<int>(object.position_reference, 8);
    const double & yaw = object.orientation;
    track_msg.position.x = object.position.x +
                           std::cos(yaw) * half_length * reference_to_center_[reference_index][0] -
                           std::sin(yaw) * half_width * reference_to_center_[reference_index][1];
    track_msg.position.y = object.position.y +
                           std::sin(yaw) * half_length * reference_to_center_[reference_index][0] +
                           std::cos(yaw) * half_width * reference_to_center_[reference_index][1];
    track_msg.position.z = object.position.z;

    track_msg.velocity = object.absolute_velocity;
    track_msg.acceleration = object.absolute_acceleration;
    track_msg.size.x = object.shape_length_edge_mean;
    track_msg.size.y = object.shape_width_edge_mean;
    track_msg.size.z = 1.f;

    uint8_t max_score = object.classification_unknown;
    track_msg.classification = UNKNOWN_ID;

    if (object.classification_car > max_score) {
      max_score = object.classification_car;
      track_msg.classification = CAR_ID;
    }
    if (object.classification_truck > max_score) {
      max_score = object.classification_truck;
      track_msg.classification = TRUCK_ID;
    }
    if (object.classification_motorcycle > max_score) {
      max_score = object.classification_motorcycle;
      track_msg.classification = MOTORCYCLE_ID;
    }
    if (object.classification_bicycle > max_score) {
      max_score = object.classification_bicycle;
      track_msg.classification = BICYCLE_ID;
    }
    if (object.classification_pedestrian > max_score) {
      max_score = object.classification_pedestrian;
      track_msg.classification = PEDESTRIAN_ID;
    }

    track_msg.position_covariance[0] = static_cast<float>(object.position_std.x);
    track_msg.position_covariance[1] = object.position_covariance_xy;
    track_msg.position_covariance[2] = 0.f;
    track_msg.position_covariance[3] = static_cast<float>(object.position_std.y);
    track_msg.position_covariance[4] = 0.f;
    track_msg.position_covariance[5] = static_cast<float>(object.position_std.z);

    track_msg.velocity_covariance[0] = static_cast<float>(object.absolute_velocity_std.x);
    track_msg.velocity_covariance[1] = object.absolute_velocity_covariance_xy;
    track_msg.velocity_covariance[2] = 0.f;
    track_msg.velocity_covariance[3] = static_cast<float>(object.absolute_velocity_std.y);
    track_msg.velocity_covariance[4] = 0.f;
    track_msg.velocity_covariance[5] = static_cast<float>(object.absolute_velocity_std.z);

    track_msg.acceleration_covariance[0] = static_cast<float>(object.absolute_acceleration_std.x);
    track_msg.acceleration_covariance[1] = object.absolute_acceleration_covariance_xy;
    track_msg.acceleration_covariance[2] = 0.f;
    track_msg.acceleration_covariance[3] = static_cast<float>(object.absolute_acceleration_std.y);
    track_msg.acceleration_covariance[4] = 0.f;
    track_msg.acceleration_covariance[5] = static_cast<float>(object.absolute_acceleration_std.z);

    track_msg.size_covariance[0] = INVALID_COVARIANCE;
    track_msg.size_covariance[1] = 0.f;
    track_msg.size_covariance[2] = 0.f;
    track_msg.size_covariance[3] = INVALID_COVARIANCE;
    track_msg.size_covariance[4] = 0.f;
    track_msg.size_covariance[5] = INVALID_COVARIANCE;

    output_msg.tracks.emplace_back(track_msg);
  }

  return output_msg;
}

visualization_msgs::msg::MarkerArray ContinentalARS548DriverRosWrapper::ConvertToMarkers(
  const continental_msgs::msg::ContinentalArs548ObjectList & msg)
{
  visualization_msgs::msg::MarkerArray marker_array;
  marker_array.markers.reserve(4 * msg.objects.size());

  constexpr int LINE_STRIP_CORNERS_NUM = 17;
  constexpr std::array<std::array<double, 3>, LINE_STRIP_CORNERS_NUM> cube_corners = {
    {{{-1.0, -1.0, -1.0}},
     {{-1.0, -1.0, 1.0}},
     {{-1.0, 1.0, 1.0}},
     {{-1.0, 1.0, -1.0}},
     {{-1.0, -1.0, -1.0}},
     {{1.0, -1.0, -1.0}},
     {{1.0, -1.0, 1.0}},
     {{1.0, 1.0, 1.0}},
     {{1.0, 1.0, -1.0}},
     {{1.0, -1.0, -1.0}},
     {{-1.0, -1.0, -1.0}},
     {{-1.0, -1.0, 1.0}},
     {{1.0, -1.0, 1.0}},
     {{1.0, 1.0, 1.0}},
     {{-1.0, 1.0, 1.0}},
     {{-1.0, 1.0, -1.0}},
     {{1.0, 1.0, -1.0}}}};

  constexpr int PALETTE_SIZE = 32;
  constexpr std::array<std::array<double, 3>, PALETTE_SIZE> color_array = {{
    {{1.0, 0.0, 0.0}},       {{0.0, 1.0, 0.0}},
    {{0.0, 0.0, 1.0}},  // Red, Green, Blue
    {{1.0, 1.0, 0.0}},       {{0.0, 1.0, 1.0}},
    {{1.0, 0.0, 1.0}},  // Yellow, Cyan, Magenta
    {{1.0, 0.647, 0.0}},     {{0.749, 1.0, 0.0}},
    {{0.0, 0.502, 0.502}},  // Orange, Lime, Teal
    {{0.502, 0.0, 0.502}},   {{1.0, 0.753, 0.796}},
    {{0.647, 0.165, 0.165}},  // Purple, Pink, Brown
    {{0.502, 0.0, 0.0}},     {{0.502, 0.502, 0.0}},
    {{0.0, 0.0, 0.502}},  // Maroon, Olive, Navy
    {{0.502, 0.502, 0.502}}, {{1.0, 0.4, 0.4}},
    {{0.4, 1.0, 0.4}},  // Grey, Light Red, Light Green
    {{0.4, 0.4, 1.0}},       {{1.0, 1.0, 0.4}},
    {{0.4, 1.0, 1.0}},  // Light Blue, Light Yellow, Light Cyan
    {{1.0, 0.4, 1.0}},       {{1.0, 0.698, 0.4}},
    {{0.698, 0.4, 1.0}},  // Light Magenta, Light Orange, Light Purple
    {{1.0, 0.6, 0.8}},       {{0.71, 0.396, 0.114}},
    {{0.545, 0.0, 0.0}},  // Light Pink, Light Brown, Dark Red
    {{0.0, 0.392, 0.0}},     {{0.0, 0.0, 0.545}},
    {{0.545, 0.545, 0.0}},                          // Dark Green, Dark Blue, Dark Yellow
    {{0.0, 0.545, 0.545}},   {{0.545, 0.0, 0.545}}  // Dark Cyan, Dark Magenta
  }};

  std::unordered_set<int> current_ids;

  radar_msgs::msg::RadarTrack track_msg;
  for (const auto & object : msg.objects) {
    const double half_length = 0.5 * object.shape_length_edge_mean;
    const double half_width = 0.5 * object.shape_width_edge_mean;
    constexpr double DEFAULT_HALF_SIZE = 1.0;
    const int reference_index = std::min<int>(object.position_reference, 8);
    const double & yaw = object.orientation;
    current_ids.emplace(object.object_id);

    visualization_msgs::msg::Marker box_marker;
    box_marker.header.frame_id = sensor_cfg_ptr_->base_frame;
    box_marker.header.stamp = msg.header.stamp;
    box_marker.ns = "boxes";
    box_marker.id = object.object_id;
    box_marker.action = visualization_msgs::msg::Marker::ADD;
    box_marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
    box_marker.lifetime = rclcpp::Duration::from_seconds(0);
    box_marker.color.r = color_array[object.object_id % PALETTE_SIZE][0];
    box_marker.color.g = color_array[object.object_id % PALETTE_SIZE][1];
    box_marker.color.b = color_array[object.object_id % PALETTE_SIZE][2];
    box_marker.color.a = 1.0;
    box_marker.scale.x = 0.1;

    box_marker.pose.position.x =
      object.position.x + std::cos(yaw) * half_length * reference_to_center_[reference_index][0] -
      std::sin(yaw) * half_width * reference_to_center_[reference_index][1];
    box_marker.pose.position.y =
      object.position.y + std::sin(yaw) * half_length * reference_to_center_[reference_index][0] +
      std::cos(yaw) * half_width * reference_to_center_[reference_index][1];
    box_marker.pose.position.z = object.position.z;
    box_marker.pose.orientation.w = std::cos(0.5 * yaw);
    box_marker.pose.orientation.z = std::sin(0.5 * yaw);

    for (const auto & corner : cube_corners) {
      geometry_msgs::msg::Point p;
      p.x = half_length * corner[0];
      p.y = half_width * corner[1];
      p.z = DEFAULT_HALF_SIZE * corner[2];
      box_marker.points.emplace_back(p);
    }

    marker_array.markers.emplace_back(box_marker);

    visualization_msgs::msg::Marker text_marker = box_marker;
    text_marker.ns = "object_age";
    text_marker.type = visualization_msgs::msg::Marker::TEXT_VIEW_FACING;
    text_marker.color.r = 1.0;
    text_marker.color.g = 1.0;
    text_marker.color.b = 1.0;
    text_marker.color.a = 1.0;
    text_marker.scale.x = 0.3;
    text_marker.scale.y = 0.3;
    text_marker.scale.z = 0.3;
    text_marker.pose.position.z += 0.5;
    text_marker.text =
      "ID=" + std::to_string(object.object_id) + " Age=" + std::to_string(object.age) + "ms";

    marker_array.markers.emplace_back(text_marker);

    std::stringstream object_status_ss;
    object_status_ss << std::fixed << std::setprecision(3) << "ID=" << object.object_id << "\n"
                     << static_cast<int>(object.status_measurement) << "/"
                     << static_cast<int>(object.status_movement) << "/"
                     << static_cast<int>(object.position_reference);

    text_marker.ns = "object_status";
    text_marker.text = object_status_ss.str();

    marker_array.markers.emplace_back(text_marker);

    std::stringstream object_dynamics_ss;
    object_dynamics_ss << std::fixed << std::setprecision(3) << "ID=" << object.object_id
                       << "\nyaw=" << object.orientation
                       << "\nyaw_rate=" << object.orientation_rate_mean
                       << "\nvx=" << object.absolute_velocity.x
                       << "\nvy=" << object.absolute_velocity.y
                       << "\nax=" << object.absolute_acceleration.x
                       << "\nay=" << object.absolute_acceleration.y;

    text_marker.ns = "object_dynamics";
    text_marker.text = object_dynamics_ss.str();

    marker_array.markers.emplace_back(text_marker);
  }

  for (const auto & previous_id : previous_ids_) {
    if (current_ids.find(previous_id) != current_ids.end()) {
      continue;
    }

    visualization_msgs::msg::Marker delete_marker;
    delete_marker.header.frame_id = sensor_cfg_ptr_->base_frame;
    delete_marker.header.stamp = msg.header.stamp;
    delete_marker.ns = "boxes";
    delete_marker.id = previous_id;
    delete_marker.action = visualization_msgs::msg::Marker::DELETE;

    marker_array.markers.push_back(delete_marker);

    delete_marker.ns = "object_age";
    marker_array.markers.push_back(delete_marker);

    delete_marker.ns = "object_status";
    marker_array.markers.push_back(delete_marker);

    delete_marker.ns = "object_dynamics";
    marker_array.markers.push_back(delete_marker);
  }

  previous_ids_.clear();
  previous_ids_ = current_ids;

  return marker_array;
}

RCLCPP_COMPONENTS_REGISTER_NODE(ContinentalARS548DriverRosWrapper)
}  // namespace ros
}  // namespace nebula
