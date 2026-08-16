#include "CGeographic.h"
#define M_PI 3.14159265358979323846
#include <cmath>
CGeographic::CGeographic(double latitude, double longitude)
	: m_Longitude(longitude), m_Latitude(latitude)
{
	m_Latitude = latitude;
	m_Longitude = longitude;
}

double CGeographic::DistanceTo(const CGeographic other) 	
{
	const double earthRadiusKm = 6371.0;
	double lat1Rad = m_Latitude * M_PI / 180.0;
	double lat2Rad = other.m_Latitude * M_PI / 180.0;
	double deltaLatRad = (other.m_Latitude - m_Latitude) * M_PI / 180.0;
	double deltaLonRad = (other.m_Longitude - m_Longitude) * M_PI / 180.0;
	double a = sin(deltaLatRad / 2) * sin(deltaLatRad / 2) +
		cos(lat1Rad) * cos(lat2Rad) *
		sin(deltaLonRad / 2) * sin(deltaLonRad / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));
	return earthRadiusKm * c*1000.0;
}
double CGeographic::BearingTo(const CGeographic other)
{
	const double lat1Rad = m_Latitude * M_PI / 180.0;
	const double lat2Rad = other.m_Latitude * M_PI / 180.0;
	const double deltaLonRad = (other.m_Longitude - m_Longitude) * M_PI / 180.0;

	const double y = sin(deltaLonRad) * cos(lat2Rad);
	const double x = cos(lat1Rad) * sin(lat2Rad) -
		sin(lat1Rad) * cos(lat2Rad) * cos(deltaLonRad);

	double bearingRad = atan2(y, x);
	double bearingDeg = bearingRad * 180.0 / M_PI;

	return fmod(bearingDeg + 360.0, 360.0);
}
