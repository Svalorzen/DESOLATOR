#include "Table.h"

#include <fstream>

Table::Table() {
    // Clear table
    for ( size_t i = 0; i < std::tuple_size<TableType>::value; i++ )
        for ( size_t j = 0; j < std::tuple_size<TransitionType>::value; j++ )
            std::get<0>(table_[i][j]) = std::get<1>(table_[i][j]) = std::get<2>(table_[i][j]) = std::get<3>(table_[i][j]) = 0.0;

    isValid_ = true;
}

void Table::update(const MDPState & before, const Action & a, const MDPState & after, double reward) {
    if ( a == Action::Attack ) {
        std::get<0>(table_[before][after])++;
        std::get<2>(table_[before][after]) += reward;
    }
    else {
        std::get<1>(table_[before][after])++;
        std::get<3>(table_[before][after]) += reward;
    }
}

bool Table::load(const std::string & filename) {
    std::ifstream file(filename.c_str(), std::ifstream::in);

    lastFilename_ = filename;

    for ( size_t i = 0; i < std::tuple_size<TableType>::value; i++ )
        for ( size_t j = 0; j < std::tuple_size<TransitionType>::value; j++ )
            if ( !(file >> std::get<0>(table_[i][j]) >> std::get<1>(table_[i][j]) >> std::get<2>(table_[i][j]) >> std::get<3>(table_[i][j]) ) ) {
                isValid_ = false;
                return false;
            }
    // Should we verify the data in some way?
    file.close();
    isValid_ = true;
    return true;
}

bool Table::save(std::string filename) {
    if ( !isValid_ ) return false;

    if ( filename == "" ) filename = lastFilename_;

    std::ofstream file(filename.c_str(), std::ofstream::out);
    int counter = 0;
    for ( size_t i = 0; i < std::tuple_size<TableType>::value; i++ ) {
        for ( size_t j = 0; j < std::tuple_size<TransitionType>::value; j++ ) {
            file << std::get<0>(table_[i][j]) << " " << std::get<1>(table_[i][j]) << " " << std::get<2>(table_[i][j]) << " " << std::get<3>(table_[i][j]) << " ";
		}
        file << "\n";
	}

    file.close();
    return true;
}
