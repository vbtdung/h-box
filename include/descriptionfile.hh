/*
Copyright (c) 2010-2012 Aalto University

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef DESCRIPTIONFILE_HH
#define DESCRIPTIONFILE_HH

#include <iostream>
#include <sstream>

using namespace std;
	
/**
 * @class xml_description_file
 * @brief Adjust the content of the description file to the Jabber username in the configuration file.
 * @author Vu Ba Tien Dung
 *
 */
class xml_description_file {
	private:
		string filename; 	// the location of the description file
		
	public:
		/**
		* Construstor of the xml_description_file class
		* The constructor takes the location of the description file
		*
		*/
		xml_description_file(const string& filename) : filename(filename) {}
		
		/**
		 * Fix the username in the root device description file
		 * @param username the new username
		 * @return return true of the content of the file is adjusted accordingly to the new username
		 *
		 */
		bool changeUDN(string& username)
		{
			string temp = "";
		
			// open the description file and retrieve the current content
			ifstream infile;
			infile.open(filename.c_str(), ifstream::in);
			if (!infile) return false; // fail
		
			// clean the resource part in the username, if needed e.g. user@jabber.com/hbox -> user@jabber.com
			int found = username.find_first_of("/");
			if (found != string::npos)
				username = username.substr(0, found);
		
			// rewrite the wrong content with a new username
			int ch = infile.get();
			while (!infile.eof()) {
				// the UDN tag is where the username is located
				if (!(temp.length() > 4 && 
					temp.at(temp.length() - 5) == '<' && temp.at(temp.length() - 4) == 'U' &&
					temp.at(temp.length() - 3) == 'D' && temp.at(temp.length() - 2) == 'N' &&
					temp.at(temp.length() - 1) == '>'))
				{
					temp += (char) ch;
					ch = infile.get();
				}
				else
				{
					// write uuid:user@jabber.com to the UDN tag
					temp += "uuid:";
					temp += username;
				
					while (!infile.eof())
					{
						ch = infile.get();
						if ((char) ch == '<') break;
					}
				}
			}

			// overwrite the file with the new content
			infile.close();
			ofstream outfile;
			outfile.open(filename.c_str());
			if (!outfile) return false;
			outfile << temp;
			outfile.close();		

			// success
			return true;
		}	
};

#endif
