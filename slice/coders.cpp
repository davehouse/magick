#include <iostream>

#include <Magick++.h>

using namespace std;
using namespace Magick;

void list_coders(bool verbose=false) {
    list<CoderInfo> coderList; 
    coderInfoList( &coderList,           // Reference to output list 
        CoderInfo::TrueMatch, // Match readable formats 
        CoderInfo::AnyMatch,  // Don't care about writable formats 
        CoderInfo::AnyMatch); // Don't care about multi-frame support 
    list<CoderInfo>::iterator entry = coderList.begin(); 

    while( entry != coderList.end() )
    {
        if ( entry->isReadable() ) {
            cout << entry->name();
            if (verbose) {
                cout << ": (" << entry->description() << "), ";
            } else {
                cout << ", ";
            }
        }
        entry++;
    }
}

int main(int argc, char **argv)
{
    list_coders();
}