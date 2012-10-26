Session, Group, and Application Keys

Session Key:
    - 32 hex characters (128-bit)
    - generated when a new NhtSession (cookie) is generated
    - stored in NhtSession structure
    - destroyed when the session is destroyed (times out)
    - provided by each application to the server whenever the application:
	* Launches or loads a new application on the same server
	* Accesses a resource such as OSML queries
	* Accesses a report or other NHT resource.
	* Pings the server
    - timeout by traditional 2-timer Centrallix NHT watchdog/expiry timers.

Group Key:
    - 16 hex characters (64-bit)
    - generated when a new application is loaded but no group key is provided
      with the application load request (URL).
    - stored in NhtAppGroup structure
    - destroyed when the app group times out
    - provided by each application to the server whenever the application:
	* Launches or loads a new application on the same server
	* Accesses a resource such as OSML queries
	* Accesses a report or other NHT resource.
	* Pings the server
    - timeout by hooking into NHT watchdog logic, have watchdog incorporate
      expiry information for groups as well as sessions.  Group expiry works
      the same - two timers.

Application Key:
    - 16 hex characters (64-bit)
    - generated each time a new application is loaded
    - stored in NhtApp structure
    - destroyed when the app times out
    - provided by each application to the server whenever the application:
	* Accesses a resource such as OSML queries
	* Accesses other NHT resources.
	* Pings the server
    - timeout by hooking into NHT watchdog logic, have watchdog incorporate
      expiry information for apps as well as sessions.  App expiry works
      the same - two timers.
    - Open an associated OSML Session for each .app

S, G, and A Keys are provided to the UI via the "akey" value in the
application.  It takes the form:

SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS-GGGGGGGGGGGGGGGG-AAAAAAAAAAAAAAAA

where each S is a hex character of the session key, and so forth.

ObjectSources:
    - No longer open their own OSML session; instead use the one associated
      with the App as a whole.  This solves the problem of dangling OSML
      sessions for objectsources -- when an app is closed but the session
      remains alive for the rest of the day inside other apps.
    - Removal of opensession and closesession from the OSRC logic, but leave
      those functions in nht_internal_OSML().
    - This does provide latency improvement, because OSRC's no longer need a
      back-and-forth handshake to establish a session before they open a 
      query.

SysInfo:
    - Move /users to /session/users
	:name (username), :session_cnt, :group_cnt, :app_cnt, :last_activity,
	:first_activity
    - Create /session/sessions
	:name (id), :last_activity, :username, :group_cnt, :app_cnt,
	:first_activity
    - Create /session/appgroups
	:name (id), :last_activity, :username, :app_cnt, :start_app_url,
	:first_activity, :session_id
    - Create /session/apps
	:name (id), :last_activity, :username, :app_url, :first_activity,
	:session_id, :group_id


Data structure changes:

NhtUser:

    added FirstActivity
    added Sessions (XArray)

NHTSession:

    changed AKey to SKey
    added S_ID integer
    added S_ID_Text[24]
    added FirstActivity
    added LastActivity
    added AppGroups (XArray)

NHTAppGroup:

    all new

NHTApp

    all new

NHT

    added SessionsByID (XHashTable)
    added [SGA]_ID_Count fields (long long)

WGTR / ClientParams

    changed AKey from 64 char to 256 char to accommodate longer akey.


