#include <napi.h>
#include <windows.h>
#include <tchar.h>

using namespace std;
using namespace Napi;

vector<pair<string, DWORD>> EnumerateValues( HKEY hkey ) {

	vector<pair<string, DWORD>> values;

	DWORD index = 0;
	TCHAR name[ MAX_PATH ] = { 0 };
	DWORD nameSize = MAX_PATH;
	DWORD type = 0;

	while( RegEnumValue( hkey, index, name, &nameSize, NULL, &type, NULL, NULL ) == ERROR_SUCCESS ) {

		values.push_back( make_pair( name, type ) );
		nameSize = MAX_PATH;
		index++;

	}

	return values;

}

vector<string> split( string str, string delimeter ) {

	vector<string> result;

	while( str.size( ) ) {

		int index = str.find( delimeter );

        if( index != string::npos ) {

			result.push_back( str.substr( 0, index ) );
			str = str.substr( index + delimeter.size( ) );
			if( str.size( ) == 0 ) result.push_back( str );

		} else {

			result.push_back( str );
			str = "";

		}
	}

	return result;

}

string strv_join( vector<string> vec, string joinee = "", int start = 0, int end = 0 ) {

	string result = "";

	if( end == 0 ) {

		end = vec.size( );

	}

	if( end < 0 ) {

		end += vec.size( );

	}

	for( int i = 0; i < vec.size( ); i++ ) {

		if( i >= start && i < end ) {

			if( i == end - 1 ) {

				result += vec[ i ];

			} else {

				result += vec[ i ] + joinee;

			}

		}

	}

	return result;

}

HKEY parseHive( string str ) {

	HKEY hive;

	if( str == "hklm" || str == "hkey_local_machine" ) {

		hive = HKEY_LOCAL_MACHINE;

	} else if( str == "hkcu" || str == "hkey_current_user" ) {

		hive = HKEY_CURRENT_USER;

	} else if( str == "hkcr" || str == "hkey_classes_root" ) {

		hive = HKEY_CLASSES_ROOT;

	} else if( str == "hku" || str == "hkey_users" ) {

		hive = HKEY_USERS;

	} else if( str == "hkcc" || str == "hkey_current_config" ) {

		hive = HKEY_CURRENT_CONFIG;

	} else {

		hive = HKEY_LOCAL_MACHINE;

	}

	return hive;

}

Value WatchPath( const CallbackInfo& info ) {

	Env env = info.Env( );

	string hiveString;
	const char* path;
	Function callback;

	switch( info.Length( ) ) {

		case 2: {

			vector<string> spt = split( info[ 0 ].As<String>( ).Utf8Value( ), ":" );
			vector<string> spt_ = split( spt[ 1 ], "\\" );

			hiveString = spt[ 0 ];
			path = strv_join( spt_, "\\" ).c_str( );

			callback = info[ 1 ].As<Function>( );

			}

			break;

		case 3:

			hiveString = info[ 0 ].As<String>( ).Utf8Value( );
			path = info[ 1 ].As<String>( ).Utf8Value( ).c_str( );

			callback = info[ 2 ].As<Function>( );

			break;

		default:

			napi_throw( env, String::New( env, "Invalid argument count" ) );

			break;

	}

	HKEY hive = parseHive( hiveString );

	HKEY hKey;
	HANDLE event;
	LONG lResult;

	lResult = RegOpenKeyEx( hive, path, 0, KEY_NOTIFY | KEY_READ, &hKey );

	if( lResult != ERROR_SUCCESS ) {

		napi_throw( env, String::New( env, "Failed to open the path supplied. EC " + to_string( lResult ) ) );

		return env.Undefined( );

	}

	vector<pair<string, DWORD>> types = EnumerateValues( hKey );

	vector<pair<string, DWORD>> dwordValues;
	vector<pair<string, ULONGLONG>> qwordValues;
	vector<pair<string, string>> szValues;
	vector<pair<string, vector<string>>> multiSzValues;

	for( int i = 0; i < types.size( ); i++ ) {

		auto key = types[ i ].first.c_str( );

		DWORD type = 0;
		DWORD size = 0;

		lResult = RegQueryValueEx( hKey, key, NULL, &type, NULL, &size );

		switch( type ) {

			case REG_SZ:

				TCHAR szValue[ MAX_PATH ];

				lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) szValue, &size );

				szValues.push_back( make_pair( key, szValue ) );

				break;

			case REG_QWORD:

				ULONGLONG qwordValue;
				size = sizeof( ULONGLONG );

				lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &qwordValue, &size );

				qwordValues.push_back( make_pair( key, qwordValue ) );

				break;

			case REG_DWORD:

				DWORD dwordValue;
				size = sizeof( DWORD );

				lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &dwordValue, &size );

				dwordValues.push_back( make_pair( key, dwordValue ) );

				break;

			case REG_MULTI_SZ:

				vector<TCHAR> mszValues;
				LPCTSTR szData;

				mszValues.resize( size / sizeof( TCHAR ) + 1 );
				szData = mszValues.data( );

				lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) szData, &size );

				vector<string> szValues;

				int i = 0;

				for( TCHAR* sz = ( TCHAR* ) szData; *sz; sz += _tcslen( sz ) + 1 ) {

					szValues.push_back( sz );

					i++;

				}

				multiSzValues.push_back( make_pair( key, szValues ) );

				break;

		}

	}

	event = CreateEvent( NULL, TRUE, FALSE, NULL );

	while( true ) {

		RegNotifyChangeKeyValue( hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, event, TRUE );

		WaitForSingleObject( event, INFINITE );

		for( int i = 0; i < dwordValues.size( ); i++ ) {

			auto key = dwordValues[ i ].first.c_str( );

			DWORD type = REG_DWORD;
			DWORD dwordValue;
			DWORD size = sizeof( DWORD );

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &dwordValue, &size );

			if( dwordValue != dwordValues[ i ].second ) {

				Object obj = Object::New( env );

				obj.Set( "name", key );
				obj.Set( "newValue", dwordValue );
				obj.Set( "oldValue", dwordValues[ i ].second );

				callback.Call( { obj } );

				dwordValues[ i ].second = dwordValue;

			}

		}

		for( int i = 0; i < qwordValues.size( ); i++ ) {

			auto key = qwordValues[ i ].first.c_str( );

			DWORD type = REG_QWORD;
			ULONGLONG qwordValue;
			DWORD size = sizeof( ULONGLONG );

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &qwordValue, &size );

			if( qwordValue != qwordValues[ i ].second ) {

				Object obj = Object::New( env );

				obj.Set( "name", key );
				obj.Set( "newValue", BigInt::New( env, qwordValue ) );
				obj.Set( "oldValue", BigInt::New( env, qwordValues[ i ].second ) );

				callback.Call( { obj } );

				qwordValues[ i ].second = qwordValue;

			}

		}

		for( int i = 0; i < szValues.size( ); i++ ) {

			auto key = szValues[ i ].first.c_str( );

			DWORD type = REG_SZ;
			TCHAR szValue[ MAX_PATH ];
			DWORD size = MAX_PATH;

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) szValue, &size );

			if( szValue != szValues[ i ].second ) {

				Object obj = Object::New( env );

				obj.Set( "name", key );
				obj.Set( "newValue", szValue );
				obj.Set( "oldValue", szValues[ i ].second );

				callback.Call( { obj } );

				szValues[ i ].second = szValue;

			}

		}

	}

	RegCloseKey( hKey );

	return env.Undefined( );

}

Value GetKeyValue( const CallbackInfo& info ) {

	Env env = info.Env( );

	string hiveString;
	const char* path;
	const char* key;

	switch( info.Length( ) ) {

		case 1: {

			vector<string> spt = split( info[ 0 ].As<String>( ).Utf8Value( ), ":" );
			vector<string> spt_ = split( spt[ 1 ], "\\" );

			hiveString = spt[ 0 ];
			path = strv_join( spt_, "\\", 0, -1 ).c_str( );
			key = spt_.back( ).c_str( );

			}

			break;

		case 2: {

			hiveString = info[ 0 ].As<String>( ).Utf8Value( );

			vector<string> spt = split( info[ 1 ].As<String>( ).Utf8Value( ), "\\" );

			path = strv_join( spt, "\\", 0, -1 ).c_str( );
			key = spt.back( ).c_str( );

			}

			break;

		case 3:

			hiveString = info[ 0 ].As<String>( ).Utf8Value( );
			path = info[ 1 ].As<String>( ).Utf8Value( ).c_str( );
			key = info[ 2 ].As<String>( ).Utf8Value( ).c_str( );

			break;

		default:

			napi_throw( env, String::New( env, "Invalid argument count" ) );

			break;

	}

	HKEY hive = parseHive( hiveString );

	HKEY hKey;
	LONG lResult;

	lResult = RegOpenKeyEx( hive, path, 0, KEY_QUERY_VALUE, &hKey );

	if( lResult != ERROR_SUCCESS ) {

		return String::New( env, path );

		napi_throw( env, String::New( env, "Failed to open the path supplied. EC " + to_string( lResult ) ) );

		return env.Undefined( );

	}
	
	DWORD type = 0;
	DWORD size = 0;

	lResult = RegQueryValueEx( hKey, key, NULL, &type, NULL, &size );

	switch( type ) {

		case REG_SZ:

			TCHAR szValue[ MAX_PATH ];

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) szValue, &size );

			RegCloseKey( hKey );

			return String::New( env, szValue );

			break;

		case REG_QWORD:

			ULONGLONG qwordValue;
			size = sizeof( ULONGLONG );

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &qwordValue, &size );

			RegCloseKey( hKey );

			return BigInt::New( env, qwordValue );

			break;

		case REG_DWORD:

			DWORD dwordValue;
			size = sizeof( DWORD );

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) &dwordValue, &size );

			RegCloseKey( hKey );

			return Number::New( env, dwordValue );

			break;

		case REG_MULTI_SZ:

			vector<TCHAR> mszValues;
			LPCTSTR szData;

			mszValues.resize( size / sizeof( TCHAR ) + 1 );
			szData = mszValues.data( );

			Array rarray = Array::New( env ); 

			lResult = RegQueryValueEx( hKey, key, NULL, &type, ( LPBYTE ) szData, &size );

			RegCloseKey( hKey );

			int i = 0;

			for( TCHAR* sz = ( TCHAR* ) szData; *sz; sz += _tcslen( sz ) + 1 ) {

				rarray.Set( i, String::New( env, sz ) );

				i++;

			}

			return rarray;

			break;

	}

	if( lResult != ERROR_SUCCESS ) {

		napi_throw( env, String::New( env, "Failed to read the path supplied. Error code " + to_string( lResult ) ) );

		RegCloseKey( hKey );

		return env.Undefined( );

	}

	RegCloseKey( hKey );

	return env.Undefined( );

}

Object Init( Env env, Object exports ) {

	exports.Set( "getKeyValue", Function::New( env, GetKeyValue ) );
	exports.Set( "watchPath", Function::New( env, WatchPath ) );

	return exports;

}

NODE_API_MODULE( NODE_GYP_MODULE_NAME, Init );
