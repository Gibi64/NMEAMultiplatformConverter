#pragma once
class CGeographic
{

public:

	CGeographic(double latitude, double logitude);
	double GetLongitude() const { return m_Longitude; }
	double GetLatitude() const { return m_Latitude; }
	double DistanceTo(const CGeographic other) ;
	double BearingTo(const CGeographic other);
private:
	double m_Longitude;
	double m_Latitude;

};

