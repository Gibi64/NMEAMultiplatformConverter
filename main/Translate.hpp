#pragma once
class CTranslate
{
public:
	static std::string TranlateRMC(CNMEATranslator::sCumulativeResult*s);
	static std::string TranslateMWV(CNMEATranslator::sCumulativeResult* s);
	static std::string TranslateDBT(CNMEATranslator::sCumulativeResult* s);
	static std::string TranslateHDT(CNMEATranslator::sCumulativeResult* s);
	static std::string TranslateCGA(CNMEATranslator::sCumulativeResult* s);
	static std::string TranslateAisAivdmType1(CNMEATranslator::sCumulativeResult* s);

};