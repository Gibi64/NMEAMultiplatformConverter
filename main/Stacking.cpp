#include "Stacking.h"
#include "CGeographic.h"
#include "CNMEATranslator.hpp"
void CStacking::StackTime(CNMEATranslator* pTranslator, void* Args)
{
	auto* R = pTranslator->GetCumulativeResult(126992);
	CTimeUtils::sUTCTime* utcTime = reinterpret_cast<CTimeUtils::sUTCTime*>(Args);
	R->m_Data[0] += utcTime->hour;
	R->m_Data[1] += utcTime->minute;
	R->m_Data[2] += utcTime->second;
	R->m_Data[3] += utcTime->millisecond;
	R->m_Count++;
	R->ReadyToSend = true;

	return ;
}
void CStacking::StackWindData(CNMEATranslator* pTranslator,void *Args)
{
	CNMEATranslator::sWindData *s = reinterpret_cast<CNMEATranslator::sWindData*>(Args);
	auto Angle = s->angleRad;
	auto Speed = s->speedMs;
	auto* R = pTranslator->GetCumulativeResult(130306);
	if (!R->m_Count)
	{
		R->m_Data[0] = Speed;
		R->m_Data[1] = Angle;
		R->m_Count = 1;
		R->StartTime = CTimeUtils::GetMs();
		R->LastTime = R->StartTime;
		return ;
	}
	R->m_Data[0] += Speed;
	R->m_Data[1] += Angle;
	R->LastTime = CTimeUtils::GetMs();
    R->m_Count++;
	R->ReadyToSend = true;

    return ;
}
void CStacking::StackDepthData(CNMEATranslator* pTranslator, void *Args)
{
	double depth = *reinterpret_cast<double*>(Args);
	auto* R = pTranslator->GetCumulativeResult(128267);
	if (!R->m_Count)
	{
		R->m_Data[0] = depth;
		R->m_Count = 1;
		R->StartTime = CTimeUtils::GetMs();
		R->LastTime = R->StartTime;
		return;
	}
	else
	{
		R->m_Data[0] += depth;
		R->LastTime = CTimeUtils::GetMs();
		R->m_Count++;
	}
	R->ReadyToSend = true;

	return;
}
void CStacking::StackSpeedData(CNMEATranslator* pTranslator, void* Args)
{
	double speed = *reinterpret_cast<double*>(Args);
	auto* R = pTranslator->GetCumulativeResult(128259);
	if (!R->m_Count)
	{
		R->m_Data[0] = speed;
		R->m_Count = 1;
		R->StartTime = CTimeUtils::GetMs();
		R->LastTime = R->StartTime;
	}
	else
	{
		R->m_Data[0] += speed;
		R->LastTime = CTimeUtils::GetMs();
		R->m_Count++;
	}
	R->ReadyToSend = true;

	return;
}

void CStacking::StackNavData(CNMEATranslator* pTranslator, void* Args)
{
	CGeographic* data = reinterpret_cast<CGeographic*>(Args);

	auto* R = pTranslator->GetCumulativeResult(129025);

	double newLat = data->GetLatitude();
	double newLon = data->GetLongitude();

	if (!R->m_Count)
	{
		R->m_Data[0] = newLat;
		R->m_Data[1] = newLon;
		R->m_Data[2] = 0;   // distance cumulée
		R->m_Data[3] = 0;   // course cumulée
		R->m_Count = 1;
		R->StartTime = CTimeUtils::GetMs();
		R->LastTime = R->StartTime;
	}
	else
	{
		double oldLat = R->m_Data[0];
		double oldLon = R->m_Data[1];

		CGeographic oldPoint(oldLat, oldLon);
		CGeographic newPoint(newLat, newLon);

		double distance = oldPoint.DistanceTo(newPoint);
		double course = oldPoint.BearingTo(newPoint);

		R->m_Data[0] = newLat;
		R->m_Data[1] = newLon;
		R->m_Data[2] += distance;
		R->m_Data[3] += course;

		R->LastTime = CTimeUtils::GetMs();
		R->m_Count++;
	}
	R->ReadyToSend = true;

}

void CStacking::StackHeadingData(CNMEATranslator* pTranslator, void* Args)
{
	double heading = *reinterpret_cast<double*>(Args);
	auto* R = pTranslator->GetCumulativeResult(127250);
	if (!R->m_Count)
	{
		R->m_Data[0] = heading;
		R->m_Count = 1;
		R->StartTime = CTimeUtils::GetMs();
		R->LastTime = R->StartTime;
	}
	else
	{
		R->m_Data[0] += heading;
		R->LastTime = CTimeUtils::GetMs();
		R->m_Count++;
	}
	R->ReadyToSend = true;

	return;
}