/**
 *  Copyright 2010 - 2012 Hewlett-Packard Development Company, L.P.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#ifndef TIMEPREFSHANDLER_H
#define TIMEPREFSHANDLER_H

#include <map>
#include <list>

#include "PrefsHandler.h"
#include "SignalSlot.h"

#define		DEFAULT_NTP_SERVER	"us.pool.ntp.org"

class TimeZoneInfo;
class PreferredZones;

//a container only
class NitzParameters
{
public:
	
	struct tm	_timeStruct;
	int _offset;
	int _dst;
	int _mcc;
	int _mnc;
	bool _timevalid;
	bool _tzvalid;
	bool _dstvalid;
	uint32_t	_localtimeStamp;
	
	NitzParameters();
	NitzParameters(struct tm& timeStruct,int offset,int dst,int mcc,int mnc,bool timevalid,bool tzvalid,bool dstvalid,uint32_t remotetimeStamp);
	
	void stampTime();
	bool valid(uint32_t timeThreshold=60);
};

class TimePrefsHandler : public PrefsHandler
					   , public Trackable
{
public:

	//DO NOT CHANGE THE VALUES!!!!
	enum {
		NITZ_TimeEnable = 1,
		NITZ_TZEnable = 2
	};
	
	enum {
		NITZ_Unknown,
		NITZ_Valid,
		NITZ_Invalid
	};
	
	TimePrefsHandler(LSPalmService* service);
	
	virtual std::list<std::string> keys() const;
	virtual bool validate(const std::string& key, json_object* value);
	virtual void valueChanged(const std::string& key, json_object* value);
	virtual json_object* valuesForKey(const std::string& key);

	json_object * timeZoneListAsJson();
	bool isValidTimeZoneName(const std::string& tzName);
	
	void postSystemTimeChange();
	void postNitzValidityStatus();
	void launchAppsOnTimeChange();
	
	const TimeZoneInfo* currentTimeZone() const { return m_cpCurrentTimeZone; }
	time_t offsetToUtcSecs() const;	
	
	void setHourFormat(const std::string& formatStr);
	
	bool isNITZTimeEnabled() { return (m_nitzSetting & NITZ_TimeEnable); }
	bool isNITZTZEnabled() { return (m_nitzSetting & NITZ_TZEnable) && m_nitzTimeZoneAvailable; }
	bool isNITZDisabled() { return ((!(m_nitzSetting & NITZ_TimeEnable)) && (!(m_nitzSetting & NITZ_TZEnable))); }
	bool setNITZTimeEnable(bool time_en);	//returns old value
	bool setNITZTZEnable(bool tz_en);	//returns old value
	
	int  getLastNITZValidity() { return m_lastNitzValidity;}
	void markLastNITZInvalid() { m_lastNitzValidity = NITZ_Invalid;}
	void markLastNITZValid() { m_lastNitzValidity = NITZ_Valid;}
	void clearLastNITZValidity() { m_lastNitzValidity = NITZ_Unknown;}

	std::list<std::string> getTimeZonesForOffset(int offset);
	
	static std::string getQualifiedTZIdFromName(const std::string& tzName);
	static std::string getQualifiedTZIdFromJson(const std::string& jsonTz);
	static std::string tzNameFromJsonValue(json_object * pValue);
	static std::string tzNameFromJsonString(const std::string& TZJson);
	
	static std::string getDefaultTZFromJson(TimeZoneInfo * r_pZoneInfo=NULL);
	
	static int getUTCTimeFromNTP(time_t& adjustedTime);
	
	static std::string transitionNITZValidState(bool nitzValid,bool userSetTime);
	
	static bool cbPowerDActivityStatus(LSHandle* lshandle, LSMessage *message,
								void *user_data);
	
	static bool cbSetSystemTime(LSHandle* lshandle, LSMessage *message,
								void *user_data);

	static bool cbSetSystemNetworkTime(LSHandle* lshandle, LSMessage *message,
				void *user_data);

	static bool cbGetSystemTime(LSHandle* lsHandle, LSMessage *message,
								void *user_data);

	static bool cbGetSystemTimezoneFile(LSHandle* lsHandle, LSMessage *message,
								void *user_data);

	static bool cbSetTimeChangeLaunch(LSHandle* lsHandle, LSMessage *message,
								void *user_data);

	static bool cbLaunchTimeChangeApps(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
	
	static bool cbGetNTPTime(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
	
	static bool cbSetPeriodicWakeupPowerDResponse(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
	
	static bool cbConvertDate(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
	
	static bool cbServiceStateTracker(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
	
	 // timeout for NITZ completion
	 static gboolean source_periodic(gpointer userData);
	 static void 	source_periodic_destroy(gpointer userData);
	    
private:

	virtual ~TimePrefsHandler();
	
	void init();
	void scanTimeZoneJson();
	
	const TimeZoneInfo* timeZone_ZoneFromOffset(int offset,int dstValue=1,int mcc=0) const;
	const TimeZoneInfo* timeZone_GenericZoneFromOffset(int offset) const;
	
	const TimeZoneInfo* timeZone_ZoneFromMCC(int mcc,int mnc) const;
	const TimeZoneInfo* timeZone_ZoneFromName(const std::string& name) const;
	
	const TimeZoneInfo* timeZone_GetDefaultZoneFailsafe();
	
	bool 				isCountryAcrossMultipleTimeZones(const TimeZoneInfo& tzinfo) const;
	
	void readCurrentNITZSettings();
	void readCurrentTimeSettings();
	
	void setTimeZone(const TimeZoneInfo * pZoneInfo);				//this one sets it in the prefs db and then calls systemSetTimeZone
	void systemSetTimeZone(const TimeZoneInfo * pZoneInfo);			//this one does the OS work to set the timezone
	static void systemSetTime(time_t utc);
	static void systemSetTime(struct timeval * pTimeVal);
	static bool jsonUtil_ZoneFromJson(json_object * json,TimeZoneInfo& r_zoneInfo);
	
	/// PIECEWISE NITZ HANDLING - (new , 11/2009)
#define NITZHANDLER_FLAGBIT_NTPALLOW		(1)
#define NITZHANDLER_FLAGBIT_MCCALLOW		(1 << 1)
#define NITZHANDLER_FLAGBIT_GZONEALLOW		(1 << 2)
#define NITZHANDLER_FLAGBIT_GZONEFORCE		(1 << 3)
#define NITZHANDLER_FLAGBIT_SKIP_DST_SELECT	(1 << 4)
#define NITZHANDLER_FLAGBIT_IGNORE_TIL_SET	(1 << 5)

#define NITZHANDLER_RETURN_ERROR			-1
#define NITZHANDLER_RETURN_SUCCESS			1
	int  nitzHandlerEntry(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int  nitzHandlerTimeValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int	 nitzHandlerOffsetValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int  nitzHandlerDstValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int  nitzHandlerExit(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	
	void  nitzHandlerSpecialCaseOffsetValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);

#define TIMEOUTFN_RESETCYCLE		1
#define TIMEOUTFN_ENDCYCLE			2
	int timeoutFunc();
	void startBootstrapCycle(int delaySeconds=20);			//this one is for machines that will never have a NITZ message kick off NTP
	void startTimeoutCycle();
	void startTimeoutCycle(unsigned int timeoutInSeconds);
	void timeout_destroy(gpointer userData);
	
	int  timeoutNitzHandlerEntry(NitzParameters& nitz,int& flags,std::string& r_statusMgs);
	int  timeoutNitzHandlerTimeValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int	 timeoutNitzHandlerOffsetValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int  timeoutNitzHandlerDstValue(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	int  timeoutNitzHandlerExit(NitzParameters& nitz,int& flags,std::string& r_statusMsg);
	
	void setPeriodicTimeSetWakeup();
	bool isNTPAllowed();
	
	void signalReceivedNITZUpdate(bool time,bool zone);
	void slotNetworkConnectionStateChanged(bool connected);

	static void dbg_time_timevalidOverride(bool&);
	static void dbg_time_tzvalidOverride(bool&);
	static void dbg_time_dstvalidOverride(bool&);

    static bool cbTelephonyPlatformQuery(LSHandle* lsHandle, LSMessage *message,
                                         void *userData);

private:

	static TimePrefsHandler * s_inst;			///not a true instance handle. Just points to the first one created
	
	typedef std::list<TimeZoneInfo*> TimeZoneInfoList;
	typedef std::list<TimeZoneInfo*>::iterator TimeZoneInfoListIterator;
	typedef std::list<TimeZoneInfo*>::const_iterator TimeZoneInfoListConstIterator;
	
	typedef std::map<int,TimeZoneInfo*> TimeZoneMap;
	typedef std::map<int,TimeZoneInfo*>::iterator TimeZoneMapIterator;
	typedef std::map<int,TimeZoneInfo*>::const_iterator TimeZoneMapConstIterator;

	typedef std::pair<int,TimeZoneInfo*> TimeZonePair;
	typedef std::multimap<int,TimeZoneInfo*> TimeZoneMultiMap;
	typedef std::multimap<int,TimeZoneInfo*>::iterator TimeZoneMultiMapIterator;
	typedef std::multimap<int,TimeZoneInfo*>::const_iterator TimeZoneMultiMapConstIterator;
	
	std::list<std::string> m_keyList;
	
	TimeZoneInfoList m_zoneList;
	TimeZoneInfoList m_syszoneList;
	
	TimeZoneMap m_mccZoneInfoMap;
	TimeZoneMap m_preferredTimeZoneMapDST;
	TimeZoneMap m_preferredTimeZoneMapNoDST;
	TimeZoneMultiMap m_offsetZoneMultiMap;
	
	static const TimeZoneInfo s_failsafeDefaultZone;
	const TimeZoneInfo * 	m_cpCurrentTimeZone;
	TimeZoneInfo *	m_pDefaultTimeZone;
    int m_nitzSetting;					//see the enum...this is a bitfield
    int m_lastNitzValidity;
    bool m_immNitzTimeValid;
    bool m_immNitzZoneValid;
    
    NitzParameters	*	m_p_lastNitzParameter;
    int					m_lastNitzFlags;
    
    static json_object * s_timeZonesJson;
    
    GSource *	m_gsource_periodic;
    guint		m_gsource_periodic_id;
    int			m_timeoutCycleCount;
    
    bool		m_sendWakeupSetToPowerD;

	time_t m_lastNtpUpdate;

    bool m_nitzTimeZoneAvailable;
};

#endif /* TIMEPREFSHANDLER_H */

