#include <cstdio>
#include <cstring>

#include <vector>
#include <sstream>
#include <string>
#include <iostream>

#include <GL/glew.h>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

extern "C" {
	#include <vecGL/shader.h>
	#include <vecGL/bones.h>
}

#include "state.hpp"
#include "view.hpp"
#include "layer.hpp"
#include "graphics.hpp"
#include "colors.hpp"
#include "colorsCustom.hpp"
#include "stringInput.hpp"
#include "triConstruct.hpp"
#include "transformOp.hpp"
#include "skeleton.hpp"

enum keyInput{
	KEY_ESC,
	KEY_QUESTION_MARK,
	
	KEY_UP,
	KEY_DOWN,
	
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	
	KEY_PLUS,
	KEY_MINUS,
	
	KEY_A,
	KEY_C,
	KEY_D,
	KEY_G,
	KEY_H,
	KEY_N,
	KEY_Q,
	KEY_S,
	
	KEY_PIPE,
	KEY_PERIOD,
	
	KEY_NONE,
	KEY_COUNT
};

enum keyInput inputToKeySymbol(sf::Keyboard::Key key){
	switch(key){
		case sf::Keyboard::Escape:
			return KEY_ESC;
		case sf::Keyboard::Slash:
			return KEY_QUESTION_MARK;
		case sf::Keyboard::Up:
			return KEY_UP;
		case sf::Keyboard::Down:
			return KEY_DOWN;
		case sf::Keyboard::Num0:
			return KEY_0;
		case sf::Keyboard::Num1:
			return KEY_1;
		case sf::Keyboard::Num2:
			return KEY_2;
		case sf::Keyboard::Num3:
			return KEY_3;
		case sf::Keyboard::Num4:
			return KEY_4;
		case sf::Keyboard::Num5:
			return KEY_5;
		case sf::Keyboard::Num6:
			return KEY_6;
		case sf::Keyboard::Num7:
			return KEY_7;
		case sf::Keyboard::Num8:
			return KEY_8;
		case sf::Keyboard::Num9:
			return KEY_9;
		case sf::Keyboard::Equal:
			return KEY_PLUS;
		case sf::Keyboard::Hyphen:
			return KEY_MINUS;
		case sf::Keyboard::A:
			return KEY_A;
		case sf::Keyboard::C:
			return KEY_C;
		case sf::Keyboard::D:
			return KEY_D;
		case sf::Keyboard::G:
			return KEY_G;
		case sf::Keyboard::H:
			return KEY_H;
		case sf::Keyboard::N:
			return KEY_N;
		case sf::Keyboard::Q:
			return KEY_Q;
		case sf::Keyboard::S:
			return KEY_S;
		case sf::Keyboard::Backslash:
			return KEY_PIPE;
		case sf::Keyboard::Period:
			return KEY_PERIOD;
		default:
			return KEY_NONE;
	}
}

#define WIN_INIT_WIDTH 800
#define WIN_INIT_HEIGHT 600

unsigned int winWidth,winHeight,maxDim;

void rescaleInnerViews(unsigned int newWidth,unsigned int newHeight){
	winWidth = newWidth;
	winHeight = newHeight;
	
	unsigned int minDim = newWidth < newHeight ? newWidth : newHeight;
	maxDim = newWidth < newHeight ? newHeight : newWidth;
	
	vw::norm::setScale((float)minDim / 2.0);
	graphics::calculateTargetDimensions();
}

int16_t randI(){
	return ((int32_t)rand() % (int32_t)INT16_MAX) * 2 + (int32_t)INT16_MIN;
}

enum mesherState newState;

class gridLayer grid;
std::vector<class vertLayer *> layers;
unsigned int currLayer = 0;

bool currLayerValid(){
	return currLayer < layers.size();
}

int main(){
	// Window --------------------------------------------
	sf::ContextSettings glSettings;
	glSettings.depthBits = 24;
	glSettings.stencilBits = 8;
	glSettings.antialiasingLevel = 4;
	glSettings.majorVersion = 3;
	glSettings.minorVersion = 3;
	
	sf::RenderWindow window(sf::VideoMode(WIN_INIT_WIDTH,WIN_INIT_HEIGHT),"Mesher",sf::Style::Default,glSettings);
	
	window.setActive(true);
	
	if(glewInit() != GLEW_OK){
		printf("GLEW initialization failed\n");
		window.close();
		
		return 0;
	}
	
	window.setActive(false);
	
	// Application-specific resources ----------------------
	if(!graphics::load(window,"share-tech-mono-regular.ttf")){
		printf("Unable to load graphical resources\n");
		window.close();
		
		return 0;
	}
	
	triCn::init();
	clrCstm::init();
	bones::init();
	
	pose::setModifiers(trOp::currentOp,trOp::dirty,trOp::valX,trOp::valY,trOp::valScalar);
	pose::reset();
	
	// View ------------------------------------------------
	window.setView(sf::View(sf::Vector2f(0.0,0.0),sf::Vector2f(WIN_INIT_WIDTH,WIN_INIT_HEIGHT)));
	vw::reset(true);
	
	sf::Vector2u winSize = window.getSize();
	rescaleInnerViews(winSize.x,winSize.y);
	
	// Global State ----------------------------------------
	srand(time(0));
	
	bool showHelp = true;
	
	bool snap = false,wireframe = false;
	class layer *nearestLayer = NULL;
	int16_t iX = 0,iY = 0,mX = 0,mY = 0;
	
	unsigned char triType = 0;
	triCn::setType(triType);
	
	unsigned char currClr = 0;
	unsigned char currBone = 0;
	
	// Temporary variables ----------------------------------
	char textBuffer[STRIN_MAX_LEN + 50];
	
	bool isCtrlDown,isAltDown,isShiftDown;
	enum keyInput keyIn;
	int statelessKI;
	
	bool selState,selActive = false;
	
	sf::Vector2<int32_t> tempPos;
	
	#define COMMAND_PARAM_COUNT 5
	// uint32_t commandParams[COMMAND_PARAM_COUNT];
	char *commandStr = NULL;
	
	char commandFeedback[STRIN_MAX_LEN + 40];
	bool commandFeedbackDisp = false;
	
	// Loop & Loop State -----------------------------------
	bool run = true;
	sf::Event event;
	
	while(window.isOpen() && run){
		// Drawing -------------------------------------
		window.clear();
		
		// Content render
		if(state::getDraw() == D_STATE_UV){
			render::tex::draw();
		}else{
			hud::ref::draw();
		}
		
		hud::drawCenter();
		
		grid.draw();
		
		for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
			if(layers.begin() + currLayer == it){
				(*it)->draw(currBone,wireframe,true);
				
				if(state::get() == STATE_ATOP_TRI_ADD){
					triCn::drawPreview(wireframe);
				}
			}else{
				(*it)->draw(currBone,wireframe,false);
			}
		}
		
		if(state::get() == STATE_BONES || state::get() == STATE_ATOP_BONE_PARENT_SET){
			bones::draw();
		}else if(state::get() == STATE_POSE || state::get() == STATE_ATOP_TRANSFORM_POSE){
			pose::draw();
		}
		
		// HUD
		trOp::drawUI();
		
		hud::drawCircleOutline(mX,mY,true,vw::norm::seekRadius(),clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_HALF));
		
		hud::drawLine(hud::LINE_VERTICAL,iX,iY,maxDim,clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_HALF));
		hud::drawLine(hud::LINE_HORIZONTAL,iX,iY,maxDim,clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_HALF));
		
		if(showHelp){
			hud::drawHelp(state::get());
		}
		
		switch(state::get()){
			case STATE_VERT_COLOR:
			case STATE_ATOP_COLOR_SET:
				hud::drawCustomColorsRef(currClr,state::get() == STATE_ATOP_COLOR_SET ? strIn::buffer() : NULL);
				
				break;
			case STATE_VERT_BONE:
			case STATE_POSE:
				hud::drawVBonesRef(currBone);
				
				break;
			case STATE_BONES:
			case STATE_ATOP_BONE_PARENT_SET:
				hud::drawBonesRef(currBone,state::get() == STATE_ATOP_BONE_PARENT_SET ? strIn::buffer() : NULL);
				
				break;
			default:
				break;
		}
		
		if(state::get() == STATE_CONSOLE){
			if(commandFeedbackDisp){
				strcpy(textBuffer,commandFeedback);
			}else{
				sprintf(textBuffer,":%s",strIn::buffer());
			}
		}else{
			sprintf(
				textBuffer,
				"zoom: %-+d | x%-8.*f",
				vw::zoomLevel(),
				vw::zoomFactor() < 1.0 ? 6 : 2,
				vw::zoomFactor()
			);
		}
		
		hud::drawBottomBar(textBuffer,snap,state::get() == STATE_ATOP_TRI_ADD,triType);
		
		hud::drawLayerNav(
			layers,
			currLayer,
			state::get() == STATE_ATOP_LAYER_NAME ? strIn::buffer() : NULL,
			&grid,
			state::get() == STATE_ATOP_GRID_SET ? strIn::buffer() : NULL
		);
		
		hud::drawStateState(state::get());
		
		// Done
		window.display();
		
		// Event Handling --------------------------------
		if(window.waitEvent(event)){
			isCtrlDown = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
			isAltDown = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);
			isShiftDown = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
			
			switch(event.type){
				case sf::Event::Closed:
					run = false;
					
					break;
				case sf::Event::Resized:
					window.setView(sf::View(sf::Vector2f(0.0,0.0),sf::Vector2f(event.size.width,event.size.height)));
					rescaleInnerViews(event.size.width,event.size.height);
					
					break;
				case sf::Event::KeyPressed:
					// State-Key Index Computation
					#define STATED_KI(stayte,ctrl,alt,shift,key) (((((int)(ctrl) << 2) | ((int)(alt) << 1) | ((int)(shift))) * TOTAL_STATE_COUNT * KEY_COUNT) + ((stayte) * KEY_COUNT) + (key))
					#define STATELESS_KI(ctrl,alt,shift,key) (((((int)(ctrl) << 2) | ((int)(alt) << 1) | ((int)(shift))) * KEY_COUNT) + (key))
					
					keyIn = inputToKeySymbol(event.key.code);
					statelessKI = STATELESS_KI(isCtrlDown,isAltDown,isShiftDown,keyIn);
					
					// Index Actuation
					if(keyIn == KEY_ESC && sf::Keyboard::isKeyPressed(sf::Keyboard::Q)){
						run = false;
					}
					
					if(state::get() < STATE_COUNT){
						switch(statelessKI){
							case STATELESS_KI(1,0,0,KEY_1):
							case STATELESS_KI(1,0,0,KEY_2):
							case STATELESS_KI(1,0,0,KEY_3):
							case STATELESS_KI(1,0,0,KEY_4):
							case STATELESS_KI(1,0,0,KEY_5):
							case STATELESS_KI(1,0,0,KEY_6):
							case STATELESS_KI(1,0,0,KEY_7):
							case STATELESS_KI(1,0,0,KEY_8):
								newState = (enum mesherState)(keyIn - KEY_1);
								
								if(state::get() == newState){
									break;
								}
								
								if(state::get() == STATE_CONSOLE){
									strIn::deactivate();
								}
								
								if(newState == STATE_CONSOLE){
									strIn::activate(STRIN_MAX_LEN);
								}
								
								state::set(newState);
								
								for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
									(*it)->nearestPoint_Clear();
								}
								
								break;
							default:
								break;
						}
					}
					
					switch(statelessKI){
						case STATELESS_KI(0,0,0,KEY_PLUS):
							vw::zoomIn();
							vw::norm::cursorCalc(window);
							
							break;
						case STATELESS_KI(0,0,0,KEY_MINUS):
							vw::zoomOut();
							vw::norm::cursorCalc(window);
							
							break;
						case STATELESS_KI(1,0,0,KEY_UP):
							if(currLayer + 1 < layers.size()){
								++currLayer;
							}
							
							break;
						case STATELESS_KI(1,0,0,KEY_DOWN):
							if(currLayer > 0){
								--currLayer;
							}
							
							break;
						case STATELESS_KI(0,0,1,KEY_QUESTION_MARK):
							showHelp = !showHelp;
							
							break;
						case STATELESS_KI(0,0,1,KEY_PIPE):
							wireframe = !wireframe;
							
							break;
						case STATELESS_KI(0,0,1,KEY_PERIOD):
							snap = !snap;
							
							break;
						default:
							break;
					}
					
					if(!state::isTextual(state::get())){
						switch(statelessKI){
							case STATELESS_KI(0,0,0,KEY_0):
								vw::reset(false);
								
								break;
							case STATELESS_KI(0,0,1,KEY_G):
								grid.visibilityToggle();
								
								break;
							case STATELESS_KI(0,0,1,KEY_H):
								if(currLayerValid()){
									layers[currLayer]->visibilityToggle();
								}
								
								break;
							case STATELESS_KI(0,0,1,KEY_S):
								hud::ref::toggleSmooth();
								
								break;
							default:
								break;
						}
					}
					
					switch(STATED_KI(state::get(),isCtrlDown,isAltDown,isShiftDown,keyIn)){
						case STATED_KI(STATE_VERT_XY,0,0,1,KEY_A):
						case STATED_KI(STATE_VERT_UV,0,0,1,KEY_A):
							if(currLayerValid()){
								layers[currLayer]->selectVert_All();
							}
							
							break;
						case STATED_KI(STATE_VERT_XY,0,0,1,KEY_C):
						case STATED_KI(STATE_VERT_UV,0,0,1,KEY_C):
							if(currLayerValid()){
								layers[currLayer]->selectVert_Clear();
							}
							
							break;
						case STATED_KI(STATE_VERT_COLOR,0,1,0,KEY_S):
							strIn::activate(8);
							state::set(STATE_ATOP_COLOR_SET);
							
							break;
						case STATED_KI(STATE_VERT_COLOR,0,0,0,KEY_UP):
						case STATED_KI(STATE_VERT_COLOR,0,0,0,KEY_DOWN):
							currClr = (COLOR_ARRAY_MAX_COUNT + currClr - (keyIn == KEY_UP) + (keyIn == KEY_DOWN)) % COLOR_ARRAY_MAX_COUNT;
							
							break;
						case STATED_KI(STATE_VERT_BONE,0,0,0,KEY_UP):
						case STATED_KI(STATE_VERT_BONE,0,0,0,KEY_DOWN):
						case STATED_KI(STATE_BONES,0,0,0,KEY_UP):
						case STATED_KI(STATE_BONES,0,0,0,KEY_DOWN):
						case STATED_KI(STATE_POSE,0,0,0,KEY_UP):
						case STATED_KI(STATE_POSE,0,0,0,KEY_DOWN):
							currBone = (BONES_MAX_COUNT + currBone - (keyIn == KEY_UP) + (keyIn == KEY_DOWN)) % BONES_MAX_COUNT;
							
							break;
						case STATED_KI(STATE_LAYERS,0,1,0,KEY_UP):
						case STATED_KI(STATE_LAYERS,0,1,0,KEY_DOWN):
							if(!sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
								break;
							}
							
							{
								struct vertLayer *newLayer = new vertLayer(100,NULL);
								newLayer->nameSet("layer");
								newLayer->vertModifiers_Set((&trOp::apply),(&trOp::dirty));
								
								if(layers.empty()){
									layers.insert(layers.begin(),newLayer);
									currLayer = 0;
								}else{
									layers.insert(layers.begin() + currLayer + (keyIn == KEY_UP),newLayer);
									currLayer += (keyIn == KEY_DOWN);
								}
							}
							
							break;
						case STATED_KI(STATE_LAYERS,0,1,0,KEY_D):
							if(!currLayerValid()){
								break;
							}
							
							delete *(layers.begin() + currLayer);
							layers.erase(layers.begin() + currLayer);
							
							if(layers.empty()){
								currLayer = 0;
							}else if(!currLayerValid()){
								currLayer = layers.size() - 1;
							}
							
							break;
						case STATED_KI(STATE_LAYERS,0,1,0,KEY_G):
							strIn::activate(4);
							state::set(STATE_ATOP_GRID_SET);
							
							break;
						case STATED_KI(STATE_LAYERS,0,1,0,KEY_N):
							if(!currLayerValid()){
								break;
							}
							
							strIn::activate(LAYER_NAME_STRLEN);
							state::set(STATE_ATOP_LAYER_NAME);
							
							break;
						case STATED_KI(STATE_BONES,0,1,0,KEY_S):
							strIn::activate(2);
							state::set(STATE_ATOP_BONE_PARENT_SET);
							
							break;
						case STATED_KI(STATE_BONES,0,1,0,KEY_C):
							bones::setParent(currBone,BONES_MAX_COUNT);
							
							break;
						case STATED_KI(STATE_POSE,0,1,0,KEY_C):
							pose::reset();
							
							break;
						case STATED_KI(STATE_ATOP_TRI_ADD,0,0,0,KEY_ESC):
							state::set(STATE_VERT_XY);
							
							break;
						case STATED_KI(STATE_ATOP_TRI_ADD,0,0,0,KEY_1):
						case STATED_KI(STATE_ATOP_TRI_ADD,0,0,0,KEY_2):
						case STATED_KI(STATE_ATOP_TRI_ADD,0,0,0,KEY_3):
							triType = keyIn - KEY_1;
							triCn::setType(triType);
							
							break;
						case STATED_KI(STATE_ATOP_TRANSFORM_XY,0,0,0,KEY_ESC):
							trOp::exit();
							state::set(STATE_VERT_XY);
							
							break;
						case STATED_KI(STATE_ATOP_COLOR_SET,0,0,0,KEY_ESC):
							strIn::deactivate();
							state::set(STATE_VERT_COLOR);
							
							break;
						case STATED_KI(STATE_ATOP_GRID_SET,0,0,0,KEY_ESC):
						case STATED_KI(STATE_ATOP_LAYER_NAME,0,0,0,KEY_ESC):
							strIn::deactivate();
							state::set(STATE_LAYERS);
							
							break;
						case STATED_KI(STATE_ATOP_BONE_PARENT_SET,0,0,0,KEY_ESC):
							strIn::deactivate();
							state::set(STATE_BONES);
							
							break;
						case STATED_KI(STATE_ATOP_TRANSFORM_POSE,0,0,0,KEY_ESC):
							trOp::exit();
							pose::clearUnappliedModifiers();
							
							state::set(STATE_POSE);
							
							break;
						default:
							break;
					}
					
					break;
				case sf::Event::TextEntered:
					switch(state::get()){
						case STATE_CONSOLE:
							commandFeedbackDisp = false;
							
							if(strIn::interpret(event.text.unicode)){
								strcpy(textBuffer,strIn::buffer());
								commandStr = strtok(textBuffer," ");
								
								if(commandStr == NULL){
									strIn::clear();
									break;
								}
								
								commandFeedbackDisp = true;
								
								if(strcmp(commandStr,"loadref") == 0){
									// Reference load -----------------------------------
									commandStr = strtok(NULL," ");
									
									if(commandStr == NULL){
										sprintf(commandFeedback,"Reference source needed");
									}else{
										if(hud::ref::load(commandStr)){
											sprintf(commandFeedback,"Loaded reference from \'%s\'",commandStr);
										}else{
											sprintf(commandFeedback,"Unable to load reference from \'%s\'",commandStr);
										}
									}
								}else if(strcmp(commandStr,"unloadref") == 0){
									// Reference unload -----------------------------------
									hud::ref::unload();
									sprintf(commandFeedback,"Reference unloaded");
									
								}else if(strcmp(commandStr,"loadtex") == 0){
									// Texture load -----------------------------------
									commandStr = strtok(NULL," ");
									
									if(commandStr == NULL){
										sprintf(commandFeedback,"Texture source needed");
									}else{
										if(render::tex::load(commandStr)){
											sprintf(commandFeedback,"Loaded texture from \'%s\'",commandStr);
										}else{
											sprintf(commandFeedback,"Unable to load texture from \'%s\'",commandStr);
										}
									}
								}else if(strcmp(commandStr,"unloadtex") == 0){
									// Texture unload -----------------------------------
									render::tex::unload();
									sprintf(commandFeedback,"Texture unloaded");
									
								}else if(strcmp(commandStr,"readlayer") == 0){
									commandStr = strtok(NULL," ");
									
									if(commandStr == NULL){
										sprintf(commandFeedback,"Layer source needed");
									}else{
										FILE *in = fopen(commandStr,"rb");
										
										if(in == NULL){
											sprintf(commandFeedback,"Unable to open \'%s\' for reading",commandStr);
										}else{
											struct vertLayer *newLayer = new vertLayer(100,in);
											newLayer->nameSet(commandStr);
											newLayer->vertModifiers_Set((&trOp::apply),(&trOp::dirty));
											
											layers.insert(layers.begin() + currLayer,newLayer);
											sprintf(commandFeedback,"Layer read from \'%s\'",commandStr);
											
											/*if(layers[currLayer]->writeLayer(out)){
												sprintf(commandFeedback,"Layer written to \'%s\'",commandStr);
											}else{
												sprintf(commandFeedback,"Unable to write layer to \'%s\'",commandStr);
											}*/
											
											fclose(in);
										}
									}
									
								}else if(strcmp(commandStr,"writelayer") == 0){
									if(!currLayerValid()){
										sprintf(commandFeedback,"No layer for writing");
										
									}else{
										commandStr = strtok(NULL," ");
										
										if(commandStr == NULL){
											sprintf(commandFeedback,"Layer destination needed");
										}else{
											FILE *out = fopen(commandStr,"wb");
											
											if(out == NULL){
												sprintf(commandFeedback,"Unable to open \'%s\' for writing",commandStr);
											}else{
												if(layers[currLayer]->writeLayer(out)){
													sprintf(commandFeedback,"Layer written to \'%s\'",commandStr);
												}else{
													sprintf(commandFeedback,"Error writing layer to \'%s\'",commandStr);
												}
												
												fclose(out);
											}
										}
									}
									
								}else{
									sprintf(commandFeedback,"Unknown command");
								}
								
								strIn::clear();
							}
							
							break;
						case STATE_ATOP_COLOR_SET:
							if(strIn::interpret(event.text.unicode)){
								clrCstm::set(currClr,strtoul(strIn::buffer(),NULL,16));
								strIn::deactivate();
								
								state::set(STATE_VERT_COLOR);
							}
							
							break;
						case STATE_ATOP_GRID_SET:
							if(strIn::interpret(event.text.unicode)){
								grid.set(atoi(strIn::buffer()));
								strIn::deactivate();
								
								state::set(STATE_LAYERS);
							}
							
							break;
						case STATE_ATOP_LAYER_NAME:
							if(strIn::interpret(event.text.unicode)){
								layers[currLayer]->nameSet(strIn::buffer());
								strIn::deactivate();
								
								state::set(STATE_LAYERS);
							}
							
							break;
						case STATE_ATOP_BONE_PARENT_SET:
							if(strIn::interpret(event.text.unicode)){
								bones::setParent(currBone,atoi(strIn::buffer()));
								strIn::deactivate();
								
								state::set(STATE_BONES);
							}
							
							break;
						default:
							break;
					}
					
					break;
				case sf::Event::MouseButtonPressed:
					switch(event.mouseButton.button){
						case sf::Mouse::Right:
							vw::panBegin();
							
							break;
						case sf::Mouse::Left:
							switch(state::get()){
								case STATE_VERT_XY:
									if(isAltDown){
										if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
											if(!currLayerValid()){
												break;
											}
											
											triCn::clear();
											triCn::add(layers[currLayer],iX,iY);
											triCn::considerPoint(iX,iY);
											
											state::set(STATE_ATOP_TRI_ADD);
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)){
											if(!currLayerValid()){
												break;
											}
											
											layers[currLayer]->nearTri_Delete();
											
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::T)){
											if(trOp::init(TROP_TRANSLATE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_XY);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::R)){
											if(trOp::init(TROP_ROTATE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_XY);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
											if(trOp::init(TROP_SCALE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_XY);
											}
										}
									}else{
										if(currLayerValid()){
											selActive = true;
											selState = layers[currLayer]->selectVert_Nearest(true,true);
										}
									}
									
									break;
								case STATE_VERT_UV:
									if(isAltDown){
										if(sf::Keyboard::isKeyPressed(sf::Keyboard::T)){
											if(trOp::init(TROP_TRANSLATE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_UV);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::R)){
											if(trOp::init(TROP_ROTATE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_UV);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
											if(trOp::init(TROP_SCALE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_UV);
											}
										}
									}else{
										if(currLayerValid()){
											selState = layers[currLayer]->selectVert_Nearest(true,true);
										}
									}
									
									break;
								case STATE_VERT_COLOR:
									if(currLayerValid()){
										layers[currLayer]->nearVert_SetColor(currClr);
									}
									
									break;
								case STATE_VERT_BONE:
									if(currLayerValid()){
										layers[currLayer]->nearVert_SetBone(currBone);
									}
									
									break;
								case STATE_BONES:
									if(!isAltDown){
										break;
									}
									
									bones::setOrigin(currBone,iX,iY);
									
									break;
								case STATE_POSE:
									if(isAltDown){
										tempPos = pose::getBonePosition(currBone);
										
										if(sf::Keyboard::isKeyPressed(sf::Keyboard::T)){
											if(trOp::init(TROP_TRANSLATE,iX,iY)){
												state::set(STATE_ATOP_TRANSFORM_POSE);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::R)){
											if(trOp::init(TROP_ROTATE,tempPos.x,tempPos.y)){
												state::set(STATE_ATOP_TRANSFORM_POSE);
											}
										}else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
											if(trOp::init(TROP_SCALE,tempPos.x,tempPos.y)){
												state::set(STATE_ATOP_TRANSFORM_POSE);
											}
										}
									}
									
									break;
								case STATE_ATOP_TRI_ADD:
									if(triCn::add(layers[currLayer],iX,iY)){
										state::set(STATE_VERT_XY);
									}else{
										triCn::considerPoint(iX,iY);
									}
									
									break;
								case STATE_ATOP_TRANSFORM_XY:
									switch(trOp::currentState()){
										case TROP_STATE_PRIME:
											trOp::prime(iX,iY);
											
											break;
										case TROP_STATE_UPDATE:
											for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
												(*it)->vertModifiers_Apply();
											}
											
											trOp::exit();
											
											state::set(STATE_VERT_XY);
											
											break;
										case TROP_STATE_NONE:
										default:
											break;
									}
									
									break;
								case STATE_ATOP_TRANSFORM_UV:
									switch(trOp::currentState()){
										case TROP_STATE_PRIME:
											trOp::prime(iX,iY);
											
											break;
										case TROP_STATE_UPDATE:
											for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
												(*it)->vertModifiers_Apply();
											}
											
											trOp::exit();
											
											state::set(STATE_VERT_UV);
											
											break;
										case TROP_STATE_NONE:
										default:
											break;
									}
									
									break;
								case STATE_ATOP_TRANSFORM_POSE:
									switch(trOp::currentState()){
										case TROP_STATE_PRIME:
											trOp::prime(iX,iY);
											
											break;
										case TROP_STATE_UPDATE:
											pose::updateModifiers(true,currBone);
											trOp::exit();
											
											state::set(STATE_POSE);
											
											break;
										case TROP_STATE_NONE:
										default:
											break;
									}
									
									break;
								default:
									break;
							}
							
							break;
						default:
							break;
					}
					
					break;
				case sf::Event::MouseWheelScrolled:
					if(event.mouseWheelScroll.delta > 0){
						vw::seekIncrease();
					}else if(event.mouseWheelScroll.delta < 0){
						vw::seekDecrease();
					}
					
					break;
				case sf::Event::MouseMoved:
					// Cursor-view calculation
					vw::norm::cursorCalc(window);
					
					mX = vw::norm::toI(vw::norm::cursorPos().x);
					mY = vw::norm::toI(vw::norm::cursorPos().y);
					
					if(sf::Mouse::isButtonPressed(sf::Mouse::Right)){
						vw::panContinue();
					}
					
					// Snapping element & nearest point & mesh-space cursor position calculation
					nearestLayer = NULL;
					
					if(snap){
						grid.nearestPoint_Find(vw::seekRadius());
						
						for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
							(*it)->nearestPoint_Find(vw::seekRadius(),currBone);
						}
						
						nearestLayer = layer::withNearestPoint((class layer *)&grid,(class layer *)vertLayer::withNearestPoint(layers));
					}else if(currLayerValid()){
						layers[currLayer]->nearestPoint_Find(vw::seekRadius(),currBone);
					}
					
					if(nearestLayer != NULL){
						iX = nearestLayer->nearestPoint_X();
						iY = nearestLayer->nearestPoint_Y();
					}else{
						iX = mX;
						iY = mY;
					}
					
					// Triangle constructor preview update
					if(triCn::building()){
						triCn::considerPoint(iX,iY);
					}
					
					// Tranformation operation preview update
					trOp::update(iX,iY);
					
					if(state::get() == STATE_ATOP_TRANSFORM_POSE && trOp::dirty()){
						pose::updateModifiers(false,currBone);
					}
					
					// Selection updates
					if(sf::Mouse::isButtonPressed(sf::Mouse::Left) && currLayerValid()){
						switch(state::get()){
							case STATE_VERT_XY:
								if(selActive){
									layers[currLayer]->selectVert_Nearest(false,selState);
								}
								
								break;
							case STATE_VERT_COLOR:
								layers[currLayer]->nearVert_SetColor(currClr);
								
								break;
							case STATE_VERT_BONE:
								layers[currLayer]->nearVert_SetBone(currBone);
								
								break;
							default:
								break;
						}
					}
					
					break;
				case sf::Event::MouseButtonReleased:
					switch(event.mouseButton.button){
						case sf::Mouse::Left:
							selActive = false;
							
							break;
						case sf::Mouse::Right:
							vw::panEnd();
							
							break;
						default:
							break;
					}
					
					break;
				default:
					break;
			}
			
			pose::update();
		}
	}
	
	for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
		delete *it;
	}
	
	triCn::free();
	graphics::free();
	
	window.close();
	
	return 0;
}