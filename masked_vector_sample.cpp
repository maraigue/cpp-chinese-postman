#include "masked_vector.hpp"
#include <iostream>

int main(void){
	std::vector<int> foobar;
	foobar.push_back(1);
	foobar.push_back(2);
	foobar.push_back(3);
	foobar.push_back(5);
	foobar.push_back(8);
	foobar.push_back(13);
	masked_vector<int> mv(foobar);
	
	do{
		mv.next();
		
		std::cout << "---- Subset" << std::endl;
		for(size_t i = 0; i < mv.size(); ++i){
			if(mv.has(i)){
				std::cout << " " << mv[i];
			}
		}
		std::cout << std::endl;
	}while(!(mv.emptymask()));
}

