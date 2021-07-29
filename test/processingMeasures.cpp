#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <memory> // for smart pointers
#include "assert.h"

using namespace std;
/* 
A standalone program to load measures.txt file and process the data for plotting.

Input data
=================================
Rank 0 Region smith-waterman MEASURES
features: [ 63,  ]: policy: 0 , count: 65 , total: 2.88899e-05 , time_avg: 4.4446e-07
features: [ 63,  ]: policy: 1 , count: 65 , total: 0.0157941 , time_avg: 0.000242986
features: [ 63,  ]: policy: 2 , count: 65 , total: 0.00745746 , time_avg: 0.00011473
features: [ 575,  ]: policy: 0 , count: 577 , total: 0.00198866 , time_avg: 3.44655e-06
features: [ 575,  ]: policy: 1 , count: 577 , total: 0.128456 , time_avg: 0.000222627
features: [ 575,  ]: policy: 2 , count: 577 , total: 0.0191061 , time_avg: 3.31128e-05
features: [ 1087,  ]: policy: 0 , count: 1089 , total: 0.00738975 , time_avg: 6.78581e-06
features: [ 1087,  ]: policy: 1 , count: 1089 , total: 0.132126 , time_avg: 0.000121328
features: [ 1087,  ]: policy: 2 , count: 1089 , total: 0.0259082 , time_avg: 2.37908e-05
features: [ 1599,  ]: policy: 0 , count: 1601 , total: 0.0164235 , time_avg: 1.02583e-05
features: [ 1599,  ]: policy: 1 , count: 1601 , total: 0.276638 , time_avg: 0.000172791
features: [ 1599,  ]: policy: 2 , count: 1601 , total: 0.056285 , time_avg: 3.51562e-05
features: [ 2111,  ]: policy: 0 , count: 2113 , total: 0.0324354 , time_avg: 1.53504e-05
features: [ 2111,  ]: policy: 1 , count: 2113 , total: 0.288673 , time_avg: 0.000136618
features: [ 2111,  ]: policy: 2 , count: 2113 , total: 0.0514953 , time_avg: 2.43707e-05


Output data conforming to gnuplot format:


nDiag   p0              p1              p2
32      0.000279132     0.0122014       0.0041564
288     0.00414445      0.060533        0.0485231
544     0.0116026       0.127367        0.0463004
800     0.0401769       0.280718        0.087325
1056    0.0606596       0.292991        0.130455
1312    0.0566188       0.300644        0.0775205
1568    0.0667464       0.333183        0.0906015
1824    0.0913814       0.287058        0.139254
2080    0.137255        0.350719        0.132058
2336    0.160883        0.50631         0.218054

*/

class Region
{
  public:
    Region (){};
    ~Region(){};

    typedef struct Measure {
      int       exec_count;
      double    time_total;
      Measure(int e, double t) : exec_count(e), time_total(t) {}
    } Measure;

    char     name[64];

    std::map<std::pair< std::vector<float>, int >,
              std::unique_ptr<Region::Measure> >  measures;

   bool loadPreviousMeasures(const string & filename ); 
};

// A helper function to extract numbers from a line
// from pos, to next ',' extract a cell, return the field, and new pos
static string extractOneCell(string& line, size_t& pos)
{
  size_t end=pos; 
  while (end<line.size() && line[end]!=',')
    end++; 

  // now end is end or , 
  string res= line.substr(pos, end-pos);
  //  cout<<"start pos:"<<pos << "--end pos:"<< end <<endl;
  pos=end+1; 
  return res; 
}

/*

Current data dump of measures has the following content
------begin of the file
=================================
Rank 0 Region daxpy MEASURES 
features: [ 9027,  ]: policy: 1 , count: 1 , total: 0.000241501 , time_avg: 0.000241501
features: [ 38863,  ]: policy: 0 , count: 1 , total: 0.00714597 , time_avg: 0.00714597
features: [ 61640,  ]: policy: 0 , count: 1 , total: 0.00875109 , time_avg: 0.00875109
features: [ 148309,  ]: policy: 0 , count: 1 , total: 0.00658767 , time_avg: 0.00658767
features: [ 240700,  ]: policy: 0 , count: 1 , total: 0.00978215 , time_avg: 0.00978215
features: [ 366001,  ]: policy: 1 , count: 1 , total: 0.00142485 , time_avg: 0.00142485
..
.-  
Rank 0 Region daxpy Reduce 
features: [ 9027, ]: P:1 T: 0.000241501
features: [ 38863, ]: P:0 T: 0.00714597
 
---------end of file
We only need to read in the first portion to populate the map of feature vector+policy -> measure pair<count, total>

std::map< std::pair< std::vector<float>, int >, std::unique_ptr<Region::Measure> > measures;

 * */
bool
Region::loadPreviousMeasures(const string & filename)
{
  string prev_data_file = filename;

  ifstream datafile (prev_data_file.c_str(), ios::in);

  if (!datafile.is_open())
  {
    cout<<"Region::loadPreviousMeasures() cannot find/open txt file "<< prev_data_file<<endl;
    return false;
  } 

  cout<<"loadPreviousMeasures() opens previous measure data file successfully: "<< prev_data_file <<endl;

  while (!datafile.eof())
  {
    string line;
    getline(datafile, line);
    if (line==".-")
    {
      // cout<<"Found end line, stopping.."<<endl;
      break;
    }

    // we have the line in a string anyway, just use it.
    size_t pos=0; 
    while (pos<line.size()) 
    {
      pos=line.find('[', pos);
      if (pos==string::npos) // a line without [ 
        break; 

      pos++; // skip [
      size_t end_bracket_pos=line.find(']', pos);
      if (end_bracket_pos==string::npos) // something is wrong
      {
        cerr<<"Error in Region::loadPreviousMeasures(). cannot find ending ] in "<< line <<endl;
        return false; 
      }

      // extract one or more features, separated by ',' between pos and end_bracket_pos
      size_t end=pos;
      vector<float> cur_features; 
      while (pos<end_bracket_pos)
      {
        string cell=extractOneCell(line, pos); 
        // may reaching over ] , skip them
        // only keep one extracted before ']'
        if (pos < end_bracket_pos)
        {
          //  cout<<stoi(cell) << " " ; // convert to integer, they are mostly sizes of things right now
          cur_features.push_back(stof(cell));
        }
        else
          break; 
      }

      // extract policy
      int cur_policy;
      pos = line.find("policy:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=7;
      string policy_id=extractOneCell(line, pos);
      //cout<<"policy:"<<stoi(policy_id) <<" "; // convert to integer, they are mostly sizes of things right now
      cur_policy = stoi(policy_id);

      // extract count
      int cur_count; 
      pos = line.find("count:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=6;
      string count=extractOneCell(line, pos);
      //cout<<"count:"<<stoi(count) <<" "; // convert to integer, they are mostly sizes of things right now
      cur_count = stoi(count);

      //extract total execution time
      pos = line.find("total:", end_bracket_pos);
      if (pos==string::npos)
        return false;
      pos+=6;
      string total=extractOneCell(line, pos);
      // cout<<"total exe time:"<<stof(total) <<endl; // convert to integer, they are mostly sizes of things right now
      float cur_total_time = stof(total); 


      auto iter = measures.find({cur_features, cur_policy});
      if (iter == measures.end()) {
        iter = measures
          .insert(std::make_pair(
                std::make_pair(cur_features, cur_policy),
                std::move(
                  std::make_unique<Region::Measure>(cur_count, cur_total_time))))
          .first;
      } else {
        // we dont' really expect anything going this branch
        // the previous training data should already be aggregated data. 
        assert (false);
        iter->second->exec_count+=cur_count;
        iter->second->time_total += cur_total_time;
      }
    } // hande a line 
  } // end while

  cout<<"After insertion: measures.size():"<<measures.size() <<endl;

  datafile.close();

  return true;
}

int main (int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s your-measures.txt\n", argv[0]);
    return 0;
  }

  string filename(argv[1]); 
  Region r; 
  r.loadPreviousMeasures (filename);

  return 0; 
}
