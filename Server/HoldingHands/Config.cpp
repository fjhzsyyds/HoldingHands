#include "stdafx.h"
#include "Config.h"
#include <fstream>
#include <cctype>
#include <string.h>
#include "json\json.h"

using std::fstream;

CConfig::~CConfig()
{

}

bool CConfig::LoadConfig(const string& config_file_path){
	fstream fs(config_file_path, fstream::in);
	if (!fs.is_open()){
		return false;
	}

	Json::Value root;
	if (!Json::Reader().parse(fs, root)){
		return false;
	}
	Json::Value::Members keys = root.getMemberNames();

	for (auto it : keys){
		map<string, string> mp;
		Json::Value::Members subkeys = root[it].getMemberNames();

		for (auto name : subkeys){
			mp[name] = root[it][name].asString();
		}
		m_config[it] = std::move(mp);
	}
	return true;
}

bool CConfig::SaveConfig(const string & config_file_path){
	fstream fs(config_file_path, fstream::out);
	if (!fs.is_open()){
		return false;
	}
	Json::Value  res;
	for (auto & key : m_config){
		Json::Value v;
		for (auto & subkey : key.second){
			v[subkey.first] = subkey.second;
		}
		res[key.first] = v;
	}
	fs << res;
	return true;
}

void CConfig::deafult_config(){
	if (m_config.size()){
		m_config.clear();
	}
	////server....
	m_config["server"]["listen_port"] = "10086";
	m_config["server"]["max_connections"] = "10000";
	m_config["server"]["modules"] = "modules";

	////File transfer;
	m_config["filetrans"]["overwrite_file"] = "true";

	////Remote Desktop
	m_config["remote_desktop"]["record_save_path"] = "data\\screenshot\\records";
	m_config["remote_desktop"]["screenshot_save_path"] = "data\\remotedesktop\\screenshot";
	////Camera...
	m_config["cam"]["record_save_path"] = "data\\cam\\records";
	m_config["cam"]["screenshot_save_path"] = "data\\cam\\screenshot";
}

const string& CConfig::getconfig(const string& key, const string&subkey){
	auto cfg = m_config.find(key);
	if (cfg == m_config.end()){
		return m_null_config;
	}

	auto value = cfg->second.find(subkey);
	if (value == cfg->second.end()){
		return m_null_config;
	}
	return value->second;
}

void CConfig::setconfig(const string&key, const string&subkey, const string&value){
	m_config[key][subkey] = value;
}