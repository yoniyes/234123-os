/*
 * test_utils.hpp
 *
 *  Created on: Oct 25, 2016
 *      Author: itay887
 */

#ifndef TEST_UTILS_HPP_
#define TEST_UTILS_HPP_
#include <cstdlib>
#include <string>
#include <cstdio>
#include <map>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

#define INPUT_FILE "settings.txt"

#define COLOR_BLUE "\033[0;34m"
#define COLOR_RED "\033[0;31m"
#define NC "\033[0m"
using std::string;

class TestUtils{
private:
	std::map<string,string> vParams;
	bool mDebug;
	string mFileName;

		void PrintWithColor(const string&,const string& str);
public:

		TestUtils():vParams(),mDebug(false),mFileName(INPUT_FILE){};
		TestUtils(const string& fileName):vParams(),mDebug(false),mFileName(fileName){};
		void Tokenize(const string& str,char delim,std::vector<string>& tokens);
		void Init();
		void LoadToIntVec(const string& str,std::vector<int>& vec);
		int ParamToInt(const string& str);
		bool ParamToBool(const string& str);
		string ParamToString(const string& param);
		void LoadToStringVec(const string& param,std::vector<string>& vec);
		void PrintDebug(const string& str);
		void PrintParameters(const string&);
		
		int ReadFromFile(const string& parameter);
		
		template<class T>
void WriteToFile(const string& parameter,T* values,unsigned int length){
	
	std::ofstream ofs("BUFFER.txt",std::ofstream::out | std::ofstream::trunc);
	std::stringstream ss;
	if(ofs.fail()){
		std::cout<<"\nError opening file BUFFER.txt\nExiting...\n";
		exit(1);
	}
	ss<<parameter<<"=";
	for(int i=0;i<length;++i){
		ss<<(values[i])<<",";
	}
	ofs<<ss.str().substr(0,ss.str().length()-1);
	ofs.close();
}
	
	int ReadFromFile(const string& parameter,std::vector<int>& values);
};



#endif /* TEST_UTILS_HPP_ */
