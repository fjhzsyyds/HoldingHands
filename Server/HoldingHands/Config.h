#pragma once
#include <map>
#include <string>

using std::string;
using std::map;

/*
config format....
#
key = value

.....这东西直接拿json就可以了.....
*/

class CConfig
{
private:
	map<string, map<string,string>> m_config;
	string	m_config_file_path;
	string  m_null_config;				//没找到的时候就返回这个。。。
	void deafult_config();
public:
	bool LoadConfig(const string& config_file_path);
	bool CConfig::SaveConfig(const string & config_file_path);
	
	const string& getconfig(const string& key, const string&subkey);
	void setconfig(const string&key, const string&subkey, const string&value);
	CConfig(){
		deafult_config();
	}
	CConfig(const string & config_file_path){
		if (!LoadConfig(config_file_path)){
			deafult_config();
		}
	}
	~CConfig();
};

