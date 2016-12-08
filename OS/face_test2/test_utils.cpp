/*
 * test_utils.cpp
 *
 *  Created on: Oct 25, 2016
 *      Author: itay887
 */

#include "test_utils.hpp"

#define SMALL_SKIP 16
#define MAJOR_SKIP 40

void TestUtils::Tokenize(const string& str,char delim,std::vector<string>& tokens)
{
	int i = 0;
	int pos = str.find(delim);
	while (pos != string::npos) {
		tokens.push_back(str.substr(i, pos-i));
		i = ++pos;
		pos = str.find(delim, pos);

		if (pos == string::npos)
			tokens.push_back(str.substr(i, str.length()));
	}
}
// template<class T>
// void WriteToFile(const string& parameter,std::vector<T> values)
// {
	// std::ofstream ofs("BUFFER.txt",std::ofstream::out | std::ofstream::trunc);
	// std::stringstream ss;
	// if(ifs.fail()){
		// std::cout<<"\nError opening file BUFFER.txt\nExiting...\n";
		// exit(1);
	// }
	// ss<<parameter<<"=";
	// for(std::vector<T>::iterator it=numbers.begin();it<numbers.end();++it){
		// ss<<(*it)<<",";
	// }
	// ofs<<ss.str().substr(0,ss.str().length()-1);
	// ofs.close();
// }

int TestUtils::ReadFromFile(const string& parameter,std::vector<int>& values)
{
	std::ifstream ifs("BUFFER.txt");
	if(ifs.fail()){
		std::cout<<"\nError opening file BUFFER.txt\nExiting...\n";
		exit(1);
	}
	string input;
	std::getline(ifs,input);
	std::vector<string> fields;
	std::vector<string> strVals;
	Tokenize(input,'=',fields);
	if(fields[0]==parameter){
	Tokenize(fields[1],',',strVals);
	for(std::vector<string>::iterator it=strVals.begin();it<strVals.end();++it){
		values.push_back(atoi((*it).c_str()));
	}
	return 0;
	}
	return -1;
}

int TestUtils::ReadFromFile(const string& parameter)
{
	std::ifstream ifs("BUFFER.txt");
	if(ifs.fail()){
		std::cout<<"\nError opening file BUFFER.txt\nExiting...\n";
		exit(1);
	}
	string input;
	std::getline(ifs,input);
	std::vector<string> fields;
	Tokenize(input,'=',fields);
	return atoi(fields[0].c_str());
}


void TestUtils::Init()
{
	std::ifstream ifs(mFileName.c_str());
	if(ifs.fail()){
		std::cout<<"\nError opening file \""<<mFileName<<"\"\nExiting...\n";
		exit(1);
	}
	string input;
	std::vector<string> fields;
	while(std::getline(ifs,input)){
		Tokenize(input,'=',fields);
		vParams[fields[0]]=fields[1];
		fields.clear();
	}
	std::map<string,string>::iterator deb=vParams.find("Debug");
	mDebug=(deb!=vParams.end() && atoi((*deb).second.c_str())>0) ? true : false;
}

void TestUtils::LoadToIntVec(const string& param,std::vector<int>& vec)
{
	std::vector<string> numbers;
	string str=vParams[param];
	Tokenize(str,',',numbers);
	for(std::vector<string>::iterator it=numbers.begin();it<numbers.end();++it){
		vec.push_back(atoi((*it).c_str()));
	}
}

/**
 * Parameter converters
 */
int TestUtils::ParamToInt(const string& str){return atoi(vParams[str].c_str());};
bool TestUtils::ParamToBool(const string& str){return ParamToInt(str)>0 ? true : false;}
string TestUtils::ParamToString(const string& param){return vParams[param];}

void TestUtils::LoadToStringVec(const string& param,std::vector<string>& vec){
	std::vector<string> numbers;
		string str=vParams[param];
		Tokenize(str,',',numbers);
		for(std::vector<string>::iterator it=numbers.begin();it<numbers.end();++it){
			vec.push_back(*it);
		}

}

void TestUtils::PrintWithColor(const string& color,const string& str)
{
	std::cout<<color<<str<<NC<<std::endl;
}
/**
 * Print only in case when debug mode is on
 */
void TestUtils::PrintDebug(const string& str)
{
	if(mDebug){
		std::stringstream ss;
		ss<<"##  "<<str<<"  ##";
		PrintWithColor(COLOR_BLUE,ss.str());
	}
}
/**
 *
 */
void TestUtils::PrintParameters(const string& test_name)
{
	std::stringstream ss;
	ss<<"-----------------------------------------------------------------------------\n";
	ss<<"Test Parameters loaded from file "<<mFileName<<" to test "<<test_name<<std::endl;
	ss<<std::setw(SMALL_SKIP)<<std::right<<"Parameter Name"<<std::setw(MAJOR_SKIP)<<"Parameter Value"<<std::setw(SMALL_SKIP)<<"\n";
	ss<<"-----------------------------------------------------------------------------";
	PrintWithColor(COLOR_BLUE,ss.str());
	ss.str("");
	for(std::map<string,string>::iterator it=vParams.begin();it!=vParams.end();++it){
		ss<<std::setw(SMALL_SKIP)<<(*it).first<<std::setw(MAJOR_SKIP)<<(*it).second<<std::setw(SMALL_SKIP);
		PrintWithColor(COLOR_BLUE,ss.str());
		ss.str("");
	}
	ss<<"-----------------------------------------------------------------------------\n";
	PrintWithColor(COLOR_BLUE,ss.str());


}
