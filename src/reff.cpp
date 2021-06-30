#include <cstdint>
#include <vector>

#include <SFML/Graphics.hpp>

#include "reff.hpp"

namespace reffs{
	struct reference{
		bool isLoaded;
		
		sf::Texture texture;
		sf::Sprite sprite;
		
		std::string path;
		int32_t x,y,scale,rotation;
		
		reference(std::string imageLocation)
		:isLoaded(false),path(imageLocation),x(0.0),y(0.0),scale(1.0),rotation(0.0){
			isLoaded = texture.loadFromFile(path);
			
			if(isLoaded){
				sprite.setTexture(texture,true);
				sprite.setOrigin(sf::Vector2f(texture.getSize()) / 2.f);
			}
		}
	};
	
	std::vector<struct reference *> references;
	
	bool add(std::string imageLocation){
		struct reference *newReff = new reference(imageLocation);
		
		if(!newReff->isLoaded){
			delete newReff;
			return false;
		}
		
		references.push_back(newReff);
		return true;
	}
	
	void clear(){
		for(std::vector<struct reference *>::iterator IT = references.begin();IT != references.end();++IT){
			delete *IT;
		}
		
		references.clear();
	}
	
	void draw(){
		
	}
}