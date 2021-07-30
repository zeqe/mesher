#include <cmath>

#include <iostream>
#include <string>

#include <GL/glew.h>

extern "C" {
	#include <vecGL/bindables.h>
	#include <vecGL/shader.h>
	#include <vecGL/bones.h>
}

#include "graphics.hpp"
#include "view.hpp"
#include "colorsCustom.hpp"
#include "geometry.hpp"
#include "skeleton.hpp"

#define STRINGIFY_DEEP(M) #M
#define STRINGIFY(M) STRINGIFY_DEEP(M)

#define PI 3.14159265358979323846

// Shader customization
const char SHADER_VERT_CASES[] =
	"case 1u:\n"
		"XY = (position + xy * fParamsV.xy) * ssr.xy * rotater(ssr.z);\n"
		"UV = uv;\n"
		
		"RGBA = indexColor(tbc.z);\n"
		// "RGBA = gl_VertexID == iParamsV.x ? vec4(RGBA.rgb,1.0) : RGBA;\n"
		
		// "RGBA = indexColor(tbc.z);\n"
		// "RGBA = iParamsV.x > 0 && gl_VertexID == iParamsV.y ? vec4(1.0 - RGBA.r,1.0 - RGBA.g,1.0 - RGBA.b,RGBA.a) : RGBA;\n"
		
		"break;\n"
	"case 2u:\n"
		"XY = (position + ((uv * 2.0) - vec2(1.0,1.0)) * fParamsV.xy) * ssr.xy * rotater(ssr.z);\n"
		"UV = uv;\n"
		
		"RGBA = indexColor(tbc.z);\n"
		// "RGBA = gl_VertexID == iParamsV.x ? vec4(RGBA.rgb,1.0) : RGBA;\n"
		
		"break;\n"
	"case 3u:\n"
		"XY = (position + indexPosition(tbc.y) * fParamsV.xy) * ssr.xy * rotater(ssr.z);\n"
		"UV = uv;\n"
		
		"RGBA = indexColor(tbc.z);\n"
		// "RGBA = gl_VertexID == iParamsV.x ? vec4(RGBA.rgb,1.0) : RGBA;\n"
		
		"break;\n"
;

const char SHADER_FRAG_CASES[] =
	"case 1u:\n"
		"fragColor = RGBA;\n"
		"break;\n"
	"case 2u:\n"
		"fragClip();\n"
		"fragColor = RGBA * texture(sampler0,vec2(UV.x,1.0 - UV.y));\n"
		"break;\n"
;

// Target
sf::RenderTarget *target;
float viewWidth,viewHeight;
float minX,maxX,minY,maxY;

// Text
sf::Font hudFont;
sf::Text hudText;

#define HUD_CHAR_SIZE 18

float charWidth,charHeight,charVDiff;
char textBuffer[50];

// Shapes
enum markType{
	MARK_CIRCLE,
	MARK_RHOMBUS,
	MARK_SQUARE,
	MARK_TRIANGLE,
	
	MARK_COUNT
};

sf::RectangleShape center,vLine,hLine;
sf::RectangleShape hudBack,hudHilight,colorBack;

sf::CircleShape circle,circleOutline;
sf::ConvexShape triangle,rhombus,square;
sf::Shape *marks[MARK_COUNT];

sf::ConvexShape stem;

sf::ConvexShape wedgeCone,wedgeCap;

#define CENTER_DIM 15.0
#define LINE_WIDTH 2.0

// Specifics
#define STATE_SYMBOL_WIDTH 2
#define STATE_SYMBOL_UNIT_WIDTH (STATE_SYMBOL_WIDTH + 1)

const std::string mesherStateSymbols[STATE_COUNT] = {
	"xy ", // STATE_VERTS
	"tr ", // STATE_TRIS
	"uv ", // STATE_V_UVS
	"vc ", // STATE_V_COLORS
	"vb ", // STATE_V_BONES
	"la ", // STATE_LAYERS
	"bo ", // STATE_BONES
	"po ", // STATE_POSE
	"co "  // STATE_CONSOLE
};

#define STATE_TITLE_MAX_LEN 18

const std::string mesherStateTitles[TOTAL_STATE_COUNT] = {
	"Vertex XY", // STATE_VERTS
	"Triangles", // STATE_TRIS
	"Vertex UV", // STATE_V_UVS
	"Vertex Colour", // STATE_V_COLORS
	"Vertex Bone", // STATE_V_BONES
	"Layers", // STATE_LAYERS
	"Bones", // STATE_BONES
	"Pose", // STATE_POSE
	"Console",  // STATE_CONSOLE
	"---", // STATE_COUNT
	"<Add Tri>", // STATE_ATOP_TRI_ADD
	"<Transform>", // STATE_ATOP_TRANSFORM
	"<Color Set>", // STATE_ATOP_COLOR_SET
	"<Grid Set>", // STATE_ATOP_GRID_SET
	"<Layer Name>", // STATE_ATOP_LAYER_NAME
	"<Parent Set>", // STATE_ATOP_BONE_PARENT_SET
	"<Transform>" // STATE_ATOP_POSE_TRANSFORM
};

std::string mesherHelpTitleUnderline = std::string(40,' ');

std::string mesherHelpGeneralTitle = "Global Controls";

std::string mesherHelpGenerals[4] = {
	// All
	"q+esc:      quit\n"
	"ctrl+[1-9]: change mode\n"
	"\n"
	"RM:    pan              ctrl+up:     move to layer above\n"
	"0:     reset view       ctrl+down:   move to layer below\n"
	"+:     zoom in\n"
	"-:     zoom out\n"
	"\n"
	"shift+?: toggle help    shift+g: hide/show grid\n"
	"shift+|: toggle snap    shift+h: hide/show current layer\n"
	, // Textual
	"q+esc:      quit\n"
	"ctrl+[1-9]: change mode\n"
	"\n"
	"RM:    pan              ctrl+up:     move to layer above\n"
	"+:     zoom in          ctrl+down:   move to layer below\n"
	"-:     zoom out\n"
	"\n"
	"shift+?: toggle help\n"
	"shift+|: toggle snap\n"
	, // Atop
	"q+esc: quit\n"
	"esc:   exit mode\n"
	"\n"
	"RM:    pan              ctrl+up:     move to layer above\n"
	"0:     reset view       ctrl+down:   move to layer below\n"
	"+:     zoom in\n"
	"-:     zoom out\n"
	"\n"
	"shift+?: toggle help    shift+g: hide/show grid\n"
	"shift+|: toggle snap    shift+h: hide/show current layer\n"
	, // Textual Atop
	"q+esc: quit\n"
	"esc:   exit mode\n"
	"\n"
	"RM:    pan              ctrl+up:     move to layer above\n"
	"+:     zoom in          ctrl+down:   move to layer below\n"
	"-:     zoom out\n"
	"\n"
	"shift+?: toggle help\n"
	"shift+|: toggle snap\n"
};

const std::string mesherHelpStates[STATE_COUNT] = {
	// STATE_VERTS
	"shift+LM: select/unselect nearest\n"
	"shift+a:  select all\n"
	"shift+c:  unselect all\n"
	,
	// STATE_TRIS
	"\n"
	,
	// STATE_V_UVS
	"\n"
	,
	// STATE_V_COLORS
	"\n"
	,
	// STATE_V_BONES
	"\n"
	,
	// STATE_LAYERS
	"alt+a+up:    add layer above current\n"
	"alt+a+down:  add layer below current\n"
	"alt+d:       delete current layer\n"
	,
	// STATE_BONES
	"\n"
	,
	// STATE_POSE
	"\n"
	,
	// STATE_CONSOLE
	"\n"
};

sf::Vector2f helpGeneralDimensions[4];
sf::Vector2f helpStateDimensions[STATE_COUNT];

// Loadable texture class
class loadableTexture{
	private:
		bool loaded;
		sf::Texture texture;
		sf::Sprite sprite;
		
	public:
		loadableTexture(){
			loaded = false;
		}
		
		bool load(const char *source,bool uniformScale){
			bool thisLoaded = texture.loadFromFile(std::string(source));
			loaded = loaded || thisLoaded;
			
			if(thisLoaded){
				sprite.setTexture(texture,true);
				sprite.setOrigin(sf::Vector2f(texture.getSize()) / 2.0f);
				
				sf::Vector2u size = texture.getSize();
				
				if(uniformScale){
					float maxDim = (float)(size.x > size.y ? size.x : size.y);
					sprite.setScale(2.0 / maxDim,2.0 / maxDim);
				}else{
					sprite.setScale(2.0 / (float)size.x,2.0 / (float)size.y);
				}
			}
			
			return thisLoaded;
		}
		
		void unload(){
			loaded = false;
		}
		
		void toggleSmooth(){
			if(!loaded){
				return;
			}
			
			texture.setSmooth(!texture.isSmooth());
		}
		
		void draw(){
			if(!loaded){
				return;
			}
			
			target->draw(
				sprite,
				sf::RenderStates(
					sf::Transform().translate(
						vw::norm::transform().transformPoint(vw::norm::toD(0),vw::norm::toD(0))
					).scale(
						vw::norm::getZoomScale(),
						vw::norm::getZoomScale()
					)
				)
			);
		}
		
		unsigned int glTex(){
			return texture.getNativeHandle();
		}
};

namespace graphics{
	sf::Vector2f calculateStringDimensions(std::string in){
		sf::Vector2f dimensions(0.0,0.0);
		
		unsigned int lastAfterNewline = 0;
		
		for(unsigned int i = 0;i < in.length();++i){
			if(in[i] == '\n'){
				dimensions.y += 1.0;
				
				if(i - lastAfterNewline > dimensions.x){
					dimensions.x = i - lastAfterNewline;
				}
				
				lastAfterNewline = i + 1;
			}
		}
		
		return dimensions;
	}
	
	bool load(sf::RenderTarget &newTarget,const char *hudFontPath){
		// Target
		target = &newTarget;
		calculateTargetDimensions();
		
		// VecGL Shader customization
		target->setActive(true);
		
		if(!initShader(SHADER_VERT_CASES,SHADER_FRAG_CASES)){
			printf("Unable to load shader\n");
			return false;
		}
		
		target->setActive(false);
		
		// Text
		if(!hudFont.loadFromFile(hudFontPath)){
			return false;
		}
		
		hudText.setFont(hudFont);
		hudText.setCharacterSize(HUD_CHAR_SIZE);
		hudText.setFillColor(sf::Color(0xffffffff));
		
		charWidth = hudFont.getGlyph('_',HUD_CHAR_SIZE,false,0.0).bounds.width + hudText.getLetterSpacing();
		charHeight = (float)hudFont.getLineSpacing(HUD_CHAR_SIZE) * hudText.getLineSpacing();
		charVDiff = charHeight - HUD_CHAR_SIZE;
		
		hudText.setOrigin(0,charHeight);
		
		// General Shapes
		hudBack.setFillColor(sf::Color(0x000000cc));
		hudBack.setOutlineColor(sf::Color(0xffffffff));
		hudBack.setOutlineThickness(LINE_WIDTH);
		
		hudHilight.setFillColor(sf::Color(0xffffffff));
		
		center.setSize(sf::Vector2f(CENTER_DIM,CENTER_DIM));
		center.setOrigin(sf::Vector2f(CENTER_DIM / 2.0,CENTER_DIM / 2.0));
		center.setFillColor(sf::Color(0xffffffff));
		
		vLine.setSize(sf::Vector2f(LINE_WIDTH,2.0));
		vLine.setOrigin(LINE_WIDTH / 2.0,1.0);
		
		hLine.setSize(sf::Vector2f(2.0,LINE_WIDTH));
		hLine.setOrigin(1.0,LINE_WIDTH / 2.0);
		
		// Markers: shapes whose areas are same as a unit circle of radius 1
		// Except for the triangle, which was artificially scaled by a factor to reduce apparant size
		circle.setPointCount(64);
		circle.setRadius(1.0);
		circle.setOrigin(sf::Vector2f(1.0,1.0));
		
		circleOutline.setPointCount(64);
		circleOutline.setFillColor(sf::Color(0x00000000));
		circleOutline.setOutlineThickness(LINE_WIDTH);
		
		float triangleScalar = 5.0 / 6.0;
		float triangleHalfHeight = triangleScalar * sqrt(sqrt(3) * PI) / 2.0;
		float triangleHalfBase = triangleScalar * sqrt(PI / sqrt(3));
		
		triangle.setPointCount(3);
		triangle.setPoint(0,sf::Vector2f(0.0,-triangleHalfHeight));
		triangle.setPoint(1,sf::Vector2f(-triangleHalfBase,triangleHalfHeight));
		triangle.setPoint(2,sf::Vector2f(triangleHalfBase,triangleHalfHeight));
		
		float rhombusInnerHypotenuse = sqrt(PI / 2.0);
		
		rhombus.setPointCount(4);
		rhombus.setPoint(0,sf::Vector2f(0.0,-rhombusInnerHypotenuse));
		rhombus.setPoint(1,sf::Vector2f(rhombusInnerHypotenuse,0.0));
		rhombus.setPoint(2,sf::Vector2f(0.0,rhombusInnerHypotenuse));
		rhombus.setPoint(3,sf::Vector2f(-rhombusInnerHypotenuse,0.0));
		
		float squareHalfSide = sqrt(PI) / 2.0;
		
		square.setPointCount(4);
		square.setPoint(0,sf::Vector2f(squareHalfSide,-squareHalfSide));
		square.setPoint(1,sf::Vector2f(squareHalfSide,squareHalfSide));
		square.setPoint(2,sf::Vector2f(-squareHalfSide,squareHalfSide));
		square.setPoint(3,sf::Vector2f(-squareHalfSide,-squareHalfSide));
		
		// Setting mark pointers
		marks[MARK_CIRCLE] = &circle;
		marks[MARK_RHOMBUS] = &rhombus;
		marks[MARK_SQUARE] = &square;
		marks[MARK_TRIANGLE] = &triangle;
		
		// Stem
		stem.setPointCount(4);
		stem.setPoint(0,sf::Vector2f(0.0,0.0));
		stem.setPoint(1,sf::Vector2f(0.1,1.25 * POINT_RADIUS));
		stem.setPoint(2,sf::Vector2f(1.0,0.0));
		stem.setPoint(3,sf::Vector2f(0.1,-1.25 * POINT_RADIUS));
		
		// Wedge parts
		wedgeCone.setPointCount(3);
		wedgeCone.setPoint(0,sf::Vector2f(0.0,0.0));
		wedgeCone.setPoint(1,sf::Vector2f(0.7,0.7));
		wedgeCone.setPoint(2,sf::Vector2f(0.7,-0.7));
		
		wedgeCap.setPointCount(4);
		wedgeCap.setPoint(0,sf::Vector2f(0.7,-0.7));
		wedgeCap.setPoint(1,sf::Vector2f(1.0,-1.0));
		wedgeCap.setPoint(2,sf::Vector2f(1.0,1.0));
		wedgeCap.setPoint(3,sf::Vector2f(0.7,0.7));
		
		/*wedgeCap.setPointCount(65);
		wedgeCap.setPoint(0,sf::Vector2f(0.0,0.0));
		
		for(unsigned int i = 0;i < 64;++i){
			float angle = (-PI / 2.0) + i * (PI / 63);
			
			wedgeCap.setPoint(1 + i,sf::Vector2f(cos(angle),sin(angle)));
		}*/
		
		// Help string dimensions
		for(unsigned int i = 0;i < 4;++i){
			helpGeneralDimensions[i] = calculateStringDimensions(mesherHelpGenerals[i]);
		}
		
		for(unsigned int i = 0;i < STATE_COUNT;++i){
			helpStateDimensions[i] = calculateStringDimensions(mesherHelpStates[i]);
		}
		
		return true;
	}
	
	void free(){
		endShader();
	}
	
	void calculateTargetDimensions(){
		viewWidth = target->getView().getSize().x;
		viewHeight = target->getView().getSize().y;
		
		minX = target->getView().getCenter().x - (viewWidth / 2.0);
		maxX = target->getView().getCenter().x + (viewWidth / 2.0);
		minY = target->getView().getCenter().y - (viewHeight / 2.0);
		maxY = target->getView().getCenter().y + (viewHeight / 2.0);
	}
}

namespace render{
	namespace tex{
		class loadableTexture texTex;
		
		bool load(const char *source){
			return texTex.load(source,false);
		}
		
		void unload(){
			texTex.unload();
		}
		
		void toggleSmooth(){
			texTex.toggleSmooth();
		}
		
		void draw(){
			texTex.draw();
		}
	}
	
	enum clr::profile meshPfl;
	enum clr::profile wireframePfl;
	
	void setColors(enum clr::profile mAr,enum clr::profile wAr){
		meshPfl = mAr;
		wireframePfl = wAr;
	}
	
	void loadAndDrawTris(struct vecTrisBuf *buf,struct vecTris **tris,enum mode draw,bool customClr,bool wireframe,int iParam){
		target->setActive(true);
		resetBindings();
		
		// Loading
		if(buf != NULL){
			deleteVecTris(*tris);
			*tris = loadVecTris(buf);
		}
		
		// Drawing
		if(*tris != NULL){
			useShader();
			
			vw::norm::vecGL::apply(*target,0.0,0.0,1.0,1.0);
			uniformIParamsV(iParam,0,0,0);
			
			unsigned int vertMode;
			
			switch(draw){
				case MODE_XY:
					vertMode = 1;
					
					break;
				case MODE_UV:
					vertMode = 2;
					bindTex0(tex::texTex.glTex());
					
					break;
				case MODE_POSE:
					vertMode = 3;
					pose::upload();
					
					break;
			}
			
			if(wireframe){
				// Fill
				if(customClr){
					clrCstm::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_HALF);
				}else{
					clr::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_HALF,meshPfl);
				}
				
				uniformVertFragModes(vertMode,draw == MODE_UV ? 2 : 0);
				drawVecTris(*tris);
				
				// Outline
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				glLineWidth(LINE_WIDTH);
				glEnable(GL_LINE_SMOOTH);
				
				if(customClr){
					clrCstm::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_ONE);
				}else{
					clr::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_ONE,wireframePfl);
				}
				
				uniformVertFragModes(vertMode,1);
				drawVecTris(*tris);
				
				glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			}else{
				// Normal drawing
				if(customClr){
					clrCstm::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_ONE);
				}else{
					clr::apply(clr::PFL_WHITE,CLR_WHITE_WHITE,clr::ALF_ONE,meshPfl);
				}
				
				uniformVertFragModes(vertMode,draw == MODE_UV ? 2 : 0);
				drawVecTris(*tris);
			}
		}
		
		clearBindings();
		target->setActive(false);
	}
}

namespace hud{
	// Datums ----------------------------------------------------------------------------------------------------------------------------------------------------
	sf::Vector2f charPosition(enum corner c,float x,float y){
		float newX,newY;
		
		switch(c){
			case cTR:
			case cBR:
				newX = maxX - x * charWidth;
				break;
			case cTL:
			case cBL:
				newX = minX + x * charWidth;
				break;
		}
		
		switch(c){
			case cTR:
			case cTL:
				newY = minY + y * charHeight;
				break;
			case cBR:
			case cBL:
				newY = maxY - y * charHeight;
				break;
		}
		
		return sf::Vector2f(newX,newY);
	}
	
	uint32_t markColor(unsigned int i,enum clr::alpha alf){
		return clr::get(clr::PFL_RANBW,i % CLR_RANBW_COUNT,alf);
	}
	
	enum markType markShape(unsigned int i){
		return (enum markType)((i / CLR_RANBW_COUNT) % MARK_COUNT);
	}
	
	// Drawing ----------------------------------------------------------------------------------------------------------------------------------------------------
	void drawCenter(){
		target->draw(center,sf::RenderStates(sf::Transform().translate(vw::transform().transformPoint(0.0,0.0))));
	}
	
	void drawStateState(enum mesherState state){
		hudBack.setSize(sf::Vector2f((2.0 + STATE_TITLE_MAX_LEN + STATE_COUNT * STATE_SYMBOL_UNIT_WIDTH) * charWidth,charHeight * 2.0));
		hudBack.setPosition(charPosition(cTL,0.0,0.0));
		target->draw(hudBack);
		
		if(state < STATE_COUNT){
			hudHilight.setPosition(charPosition(cTL,1.5 + STATE_TITLE_MAX_LEN + state * STATE_SYMBOL_UNIT_WIDTH,0.0));
			hudHilight.setSize(sf::Vector2f((STATE_SYMBOL_WIDTH + (state == STATE_COUNT - 1 ? 1.5 : 1.0)) * charWidth,2.0 * charHeight));
			target->draw(hudHilight);
		}
		
		for(unsigned int i = 0;i < STATE_COUNT;++i){
			hudText.setString(mesherStateSymbols[i]);
			hudText.setPosition(charPosition(cTL,2.0 + STATE_TITLE_MAX_LEN + i * STATE_SYMBOL_UNIT_WIDTH,1.5));
			
			if(i == state){
				hudText.setFillColor(sf::Color(0x000000ff));
			}else{
				hudText.setFillColor(sf::Color(0xffffffff));
			}
			
			target->draw(hudText);
		}
		
		if(state < TOTAL_STATE_COUNT){
			hudText.setString(mesherStateTitles[state]);
			hudText.setPosition(charPosition(cTL,2.0,1.5));
			hudText.setFillColor(sf::Color(0xffffffff));
			target->draw(hudText);
		}
	}
	
	void drawHelp(enum mesherState state){
		// Brief state indexation
		int isTextual = stateIsTextual(state);
		int isAtop = (state >= STATE_COUNT);
		int sI = (isAtop << 1) | isTextual;
		
		// Background -------
		sf::Vector2f backgroundDimensions(helpGeneralDimensions[sI]);
		
		if(state < STATE_COUNT){
			if(helpStateDimensions[state].x > helpGeneralDimensions[sI].x){
				backgroundDimensions.x = helpStateDimensions[state].x;
			}
			
			backgroundDimensions.y += helpStateDimensions[state].y;
		}
		
		hudBack.setSize(sf::Vector2f((4.0 + backgroundDimensions.x) * charWidth,(2.5 + backgroundDimensions.y + 2.5 + 1.5) * charHeight));
		hudBack.setPosition(charPosition(cTL,0.0,2.0) + sf::Vector2f(0.0,2.0));
		target->draw(hudBack);
		
		// Text -------------
		hudText.setFillColor(sf::Color(0xffffffff));
		
		// General help title
		hudText.setStyle(sf::Text::Underlined);
		hudText.setString(mesherHelpTitleUnderline);
		hudText.setPosition(charPosition(cTL,2.0,4.0));
		target->draw(hudText);
		
		hudText.setStyle(sf::Text::Regular);
		
		hudText.setString(mesherHelpGeneralTitle);
		hudText.setPosition(charPosition(cTL,2.0,4.0));
		target->draw(hudText);
		
		// General help content
		hudText.setString(mesherHelpGenerals[sI]);
		hudText.setPosition(charPosition(cTL,2.0,5.5));
		target->draw(hudText);
		
		if(state >= STATE_COUNT){
			return;
		}
		
		// State help title
		hudText.setStyle(sf::Text::Underlined);
		hudText.setString(mesherHelpTitleUnderline);
		hudText.setPosition(charPosition(cTL,2.0,5.5 + helpGeneralDimensions[sI].y + 1.0));
		target->draw(hudText);
		
		hudText.setStyle(sf::Text::Regular);
		
		hudText.setString(mesherStateTitles[state] + " Controls");
		hudText.setPosition(charPosition(cTL,2.0,5.5 + helpGeneralDimensions[sI].y + 1.0));
		target->draw(hudText);
		
		// State help content
		hudText.setString(mesherHelpStates[state]);
		hudText.setPosition(charPosition(cTL,2.0,5.5 + helpGeneralDimensions[sI].y + 2.5));
		target->draw(hudText);
	}
	
	void drawBottomBar(const char *line,bool snapOn,bool showTris,unsigned char currTri){
		// Background
		hudBack.setSize(sf::Vector2f(viewWidth,charHeight * 3.0));
		hudBack.setPosition(charPosition(cBL,0.0,2.0));
		target->draw(hudBack);
		
		// Text
		hudText.setString(std::string(line));
		hudText.setPosition(charPosition(cBL,2.0,0.5));
		hudText.setFillColor(sf::Color(0xffffffff));
		target->draw(hudText);
		
		// Snap state
		if(snapOn){
			hudHilight.setPosition(charPosition(cBR,3.0,2.0));
			hudHilight.setSize(sf::Vector2f(3.0 * charWidth,2.0 * charHeight));
			target->draw(hudHilight);
		}
		
		hudText.setString("*");
		hudText.setPosition(charPosition(cBR,2.0,0.5));
		hudText.setFillColor(sf::Color(snapOn ? 0x000000ff : 0xffffffff));
		target->draw(hudText);
		
		// Triangle state
		if(showTris){
			// Background
			if(currTri < 3){
				hudHilight.setPosition(charPosition(cBR,3.0 + (3 - currTri) * 3.0,2.0));
				hudHilight.setSize(sf::Vector2f(3.0 * charWidth,2.0 * charHeight));
				target->draw(hudHilight);
			}
			
			// Symbols
			const std::string TRI_SYMBOLS[3] = {
				"|>",
				"|)",
				"|("
			};
			
			for(int i = 0;i < 3;++i){
				hudText.setString(TRI_SYMBOLS[i]);
				hudText.setPosition(charPosition(cBR,2.5 + (3 - i) * 3.0,0.5));
				hudText.setFillColor(sf::Color(i == currTri ? 0x000000ff : 0xffffffff));
				target->draw(hudText);
			}
		}
	}
	
	void drawLayerNav(std::vector<class vertLayer *> &layers,unsigned int currLayer,const char *altCurrLayerName,class gridLayer *grid,const char *altGridDisplay){
		hudBack.setPosition(charPosition(cTR,6.0 + LAYER_NAME_STRLEN,-0.5));
		hudBack.setSize(sf::Vector2f((7.0 + LAYER_NAME_STRLEN) * charWidth,(layers.size() + 5.0) * charHeight));
		target->draw(hudBack);
		
		if(currLayer < layers.size()){
			hudHilight.setPosition(charPosition(cTR,6.0 + LAYER_NAME_STRLEN,1.0 + ((int)layers.size() - (int)currLayer))  + sf::Vector2f(0.0,charVDiff / 2.0));
			hudHilight.setSize(sf::Vector2f((7.0 + LAYER_NAME_STRLEN) * charWidth,charHeight));
			
			if((*(layers.begin() + currLayer))->visible()){
				hudHilight.setFillColor(sf::Color(0xffffffff));
			}else{
				hudHilight.setFillColor(sf::Color(0xffffff70));
			}
			
			target->draw(hudHilight);
			
			hudHilight.setFillColor(sf::Color(0xffffffff));
		}
		
		hudText.setString("Layers");
		hudText.setPosition(charPosition(cTR,4.0 + LAYER_NAME_STRLEN,1.5));
		hudText.setFillColor(sf::Color(0xffffffff));
		target->draw(hudText);
		
		unsigned int currI = 0;
		
		for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
			sprintf(
				textBuffer,
				"[%s",
				(currI == currLayer && altCurrLayerName != NULL) ? altCurrLayerName : (*it)->nameGet()
			);
			
			hudText.setString(std::string(textBuffer));
			hudText.setPosition(charPosition(cTR,4.0 + LAYER_NAME_STRLEN,2.0 + ((int)layers.size() - (int)currI)));
			
			if(currI == currLayer){
				hudText.setFillColor(sf::Color(0x000000ff));
			}else if((*it)->visible()){
				hudText.setFillColor(sf::Color(0xffffffff));
			}else{
				hudText.setFillColor(sf::Color(0xffffff70));
			}
			
			target->draw(hudText);
			
			++currI;
		}
		
		if(altGridDisplay == NULL){
			sprintf(textBuffer,"#grid: %4d",grid->get());
		}else{
			sprintf(textBuffer,"#grid: %s",altGridDisplay);
		}
		
		hudText.setString(std::string(textBuffer));
		hudText.setPosition(charPosition(cTR,4.0 + LAYER_NAME_STRLEN,3.5 + layers.size()));
		
		if(grid->visible()){
			hudText.setFillColor(sf::Color(0xffffffff));
		}else{
			hudText.setFillColor(sf::Color(0xffffff70));
		}
		
		target->draw(hudText);
	}
	
	void drawCustomColorsRef(unsigned char currColor,const char *altCurrColorHex){
		#define COLREF_CHLPAD 1
		#define COLREF_CHRPAD 2
		#define COLREF_CVPAD 1.5
		
		#define COLREF_ICWIDTH 16
		
		#define COLREF_IHEIGHT (CLR_RANBW_COUNT * 2)
		#define COLREF_IWIDTH (COLOR_ARRAY_MAX_COUNT / COLREF_IHEIGHT)
		
		#define COLREF_CHEIGHT (COLREF_IHEIGHT + (COLREF_CVPAD * 2.0))
		#define COLREF_CWIDTH ((COLREF_ICWIDTH * COLREF_IWIDTH) + COLREF_CHLPAD + COLREF_CHRPAD)
		
		#define COLREF_CTOP (COLREF_CHEIGHT + 2.0)
		
		#define COLREF_ICX(i) (COLREF_CWIDTH - COLREF_CHLPAD - (COLREF_ICWIDTH * ((i / COLREF_IHEIGHT) + 1)))
		#define COLREF_ICY(i) (COLREF_CTOP - COLREF_CVPAD - (i % COLREF_IHEIGHT) - 1.0)
		
		// Background
		hudBack.setPosition(charPosition(cBR,COLREF_CWIDTH,COLREF_CTOP));
		hudBack.setSize(sf::Vector2f(COLREF_CWIDTH * charWidth,COLREF_CHEIGHT * charHeight));
		target->draw(hudBack);
		
		// Items
		for(unsigned int i = 0;i < MARK_COUNT;++i){
			marks[i]->setScale(POINT_RADIUS,POINT_RADIUS);
		}
		
		sf::Shape *drawn;
		unsigned char currI = currColor & COLOR_ARRAY_INDEX_MASK;
		
		for(unsigned int i = 0;i < COLOR_ARRAY_MAX_COUNT;++i){
			// Custom Color Background
			colorBack.setPosition(charPosition(cBR,COLREF_ICX(i) + 10.5,COLREF_ICY(i) + 1.0));
			colorBack.setSize(sf::Vector2f(10.5 * charWidth,charHeight));
			colorBack.setFillColor(sf::Color((clrCstm::get(i) & 0xffffff00) | clr::getAlpha(clr::ALF_HALF)));
			target->draw(colorBack);
			
			// Current Color Highlight
			if(i == currI){
				colorBack.setPosition(charPosition(cBR,COLREF_ICX(i) + 15,COLREF_ICY(i) + 1.0));
				colorBack.setSize(sf::Vector2f(4.5 * charWidth,charHeight));
				colorBack.setFillColor(sf::Color(0xffffffff));
				target->draw(colorBack);
			}
			
			// Color ID & Hex
			if(i == currI){
				if(altCurrColorHex != NULL){
					sprintf(textBuffer,"%.9s",altCurrColorHex);
				}else{
					sprintf(textBuffer,"%08x",clrCstm::get(i));
				}
				
				hudText.setString(std::string(textBuffer));
				hudText.setPosition(charPosition(cBR,COLREF_ICX(i) + 9,COLREF_ICY(i)));
				hudText.setFillColor(sf::Color(0xffffffff));
				target->draw(hudText);
				
				sprintf(textBuffer,"%2d",i);
				
				hudText.setString(std::string(textBuffer));
				hudText.setPosition(charPosition(cBR,COLREF_ICX(i) + 14,COLREF_ICY(i)));
				hudText.setFillColor(sf::Color(0x000000ff));
				target->draw(hudText);
				
			}else{
				sprintf(textBuffer,"%2d   %08x",i,clrCstm::get(i));
				
				hudText.setString(std::string(textBuffer));
				hudText.setPosition(charPosition(cBR,COLREF_ICX(i) + 14,COLREF_ICY(i)));
				hudText.setFillColor(sf::Color(0xffffffff));
				target->draw(hudText);
			}
			
			// Mark
			drawn = marks[markShape(i)];
			drawn->setFillColor(sf::Color(markColor(i,clr::ALF_ONE)));
			target->draw(*drawn,sf::RenderStates(sf::Transform().translate(charPosition(cBR,COLREF_ICX(i) + 10.5,COLREF_ICY(i) + 0.5))));
		}
	}
	
	void drawVBonesRef(unsigned char currBone){
		#define VBNEREF_CHLPAD 1
		#define VBNEREF_CHRPAD 2
		#define VBNEREF_CVPAD 1.5
		
		#define VBNEREF_ICWIDTH 7
		
		#define VBNEREF_IHEIGHT (CLR_RANBW_COUNT * 2)
		#define VBNEREF_IWIDTH (BONES_MAX_COUNT / VBNEREF_IHEIGHT)
		
		#define VBNEREF_CHEIGHT (VBNEREF_IHEIGHT + (VBNEREF_CVPAD * 2.0))
		#define VBNEREF_CWIDTH ((VBNEREF_ICWIDTH * VBNEREF_IWIDTH) + VBNEREF_CHLPAD + VBNEREF_CHRPAD)
		
		#define VBNEREF_CTOP (VBNEREF_CHEIGHT + 2.0)
		
		#define VBNEREF_ICX(i) (VBNEREF_CWIDTH - VBNEREF_CHLPAD - (VBNEREF_ICWIDTH * ((i / VBNEREF_IHEIGHT) + 1)))
		#define VBNEREF_ICY(i) (VBNEREF_CTOP - VBNEREF_CVPAD - (i % VBNEREF_IHEIGHT) - 1.0)
		
		// Background
		hudBack.setPosition(charPosition(cBR,VBNEREF_CWIDTH,VBNEREF_CTOP));
		hudBack.setSize(sf::Vector2f(VBNEREF_CWIDTH * charWidth,VBNEREF_CHEIGHT * charHeight));
		target->draw(hudBack);
		
		// Items
		for(unsigned int i = 0;i < MARK_COUNT;++i){
			marks[i]->setScale(POINT_RADIUS,POINT_RADIUS);
		}
		
		sf::Shape *drawn;
		unsigned char currI = currBone & BONE_INDEX_MASK;
		
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			// Current Bone Highlight
			if(i == currI){
				colorBack.setPosition(charPosition(cBR,VBNEREF_ICX(i) + 6,VBNEREF_ICY(i) + 1.0));
				colorBack.setSize(sf::Vector2f(4.5 * charWidth,charHeight));
				colorBack.setFillColor(sf::Color(0xffffffff));
				target->draw(colorBack);
			}
			
			// Bone ID
			sprintf(textBuffer,"%2d",i);
			
			hudText.setString(std::string(textBuffer));
			hudText.setPosition(charPosition(cBR,VBNEREF_ICX(i) + 5,VBNEREF_ICY(i)));
			hudText.setFillColor(sf::Color(i == currI ? 0x000000ff : 0xffffffff));
			target->draw(hudText);
			
			// Mark
			drawn = marks[markShape(i)];
			drawn->setFillColor(sf::Color(markColor(i,clr::ALF_ONE)));
			target->draw(*drawn,sf::RenderStates(sf::Transform().translate(charPosition(cBR,VBNEREF_ICX(i) + 1.5,VBNEREF_ICY(i) + 0.5))));
		}
	}
	
	void drawBonesRef(unsigned char currBone,const char *altCurrBoneParent){
		#define BNEREF_CHLPAD 1
		#define BNEREF_CHRPAD 2
		#define BNEREF_CVPAD 1.5
		
		#define BNEREF_ICWIDTH 11
		
		#define BNEREF_IHEIGHT (CLR_RANBW_COUNT * 2)
		#define BNEREF_IWIDTH (BONES_MAX_COUNT / BNEREF_IHEIGHT)
		
		#define BNEREF_CHEIGHT (BNEREF_IHEIGHT + (BNEREF_CVPAD * 2.0))
		#define BNEREF_CWIDTH ((BNEREF_ICWIDTH * BNEREF_IWIDTH) + BNEREF_CHLPAD + BNEREF_CHRPAD)
		
		#define BNEREF_CTOP (BNEREF_CHEIGHT + 2.0)
		
		#define BNEREF_ICX(i) (BNEREF_CWIDTH - BNEREF_CHLPAD - (BNEREF_ICWIDTH * ((i / BNEREF_IHEIGHT) + 1)))
		#define BNEREF_ICY(i) (BNEREF_CTOP - BNEREF_CVPAD - (i % BNEREF_IHEIGHT) - 1.0)
		
		// Background
		hudBack.setPosition(charPosition(cBR,BNEREF_CWIDTH,BNEREF_CTOP));
		hudBack.setSize(sf::Vector2f(BNEREF_CWIDTH * charWidth,BNEREF_CHEIGHT * charHeight));
		target->draw(hudBack);
		
		// Items
		for(unsigned int i = 0;i < MARK_COUNT;++i){
			marks[i]->setScale(POINT_RADIUS,POINT_RADIUS);
		}
		
		sf::Shape *drawn;
		unsigned char currI = currBone & BONE_INDEX_MASK;
		
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			// Current Bone Highlight
			if(i == currI){
				colorBack.setPosition(charPosition(cBR,BNEREF_ICX(i) + 10,BNEREF_ICY(i) + 1.0));
				colorBack.setSize(sf::Vector2f(4.5 * charWidth,charHeight));
				colorBack.setFillColor(sf::Color(0xffffffff));
				target->draw(colorBack);
			}
			
			// Bone ID
			sprintf(textBuffer,"%2d",i);
			
			hudText.setString(std::string(textBuffer));
			hudText.setPosition(charPosition(cBR,BNEREF_ICX(i) + 9,BNEREF_ICY(i)));
			hudText.setFillColor(sf::Color(i == currI ? 0x000000ff : 0xffffffff));
			target->draw(hudText);
			
			// Bone Parent
			if(i == currI && altCurrBoneParent != NULL){
				sprintf(textBuffer,"<%.3s",altCurrBoneParent);
			}else{
				if(bones::getParent(i) < BONES_MAX_COUNT){
					sprintf(textBuffer,"<%2d",bones::getParent(i));
				}else{
					sprintf(textBuffer,"<--");
				}
			}
			
			hudText.setString(std::string(textBuffer));
			hudText.setPosition(charPosition(cBR,BNEREF_ICX(i) + 4,BNEREF_ICY(i)));
			hudText.setFillColor(sf::Color(0xffffffff));
			target->draw(hudText);
			
			// Mark
			drawn = marks[markShape(i)];
			drawn->setFillColor(sf::Color(markColor(i,clr::ALF_ONE)));
			target->draw(*drawn,sf::RenderStates(sf::Transform().translate(charPosition(cBR,BNEREF_ICX(i) + 5.5,BNEREF_ICY(i) + 0.5))));
		}
	}
	
	void drawCircle(int32_t x,int32_t y,bool isRadNorm,float radius,uint32_t color){
		float newRadius = isRadNorm ? vw::norm::getScale() * radius : radius;
		
		circle.setScale(newRadius,newRadius);
		circle.setFillColor(sf::Color(color));
		
		target->draw(
			circle,
			sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(vw::norm::toD_u(x),vw::norm::toD_u(y))))
		);
	}
	
	void drawCircleOutline(int32_t x,int32_t y,bool isRadNorm,float radius,uint32_t color){
		float newRadius = isRadNorm ? vw::norm::getScale() * radius : radius;
		
		circleOutline.setRadius(newRadius);
		circleOutline.setOrigin(sf::Vector2f(newRadius,newRadius));
		circleOutline.setOutlineColor(sf::Color(color));
		
		target->draw(
			circleOutline,
			sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(vw::norm::toD_u(x),vw::norm::toD_u(y))))
		);
	}
	
	void drawWedge(int32_t srcX,int32_t srcY,int32_t aX,int32_t aY,int32_t bX,int32_t bY,float radius,bool outline,uint32_t color){
		// Geometric math calculation
		double angleA = atan2(aY - srcY,aX - srcX);
		double angleB = atan2(bY - srcY,bX - srcX);
		double dist1 = fabs(angleA - angleB);
		double dist2 = fabs((angleA > 0.0 ? angleA : (2.0 * PI + angleA)) - (angleB > 0.0 ? angleB : (2.0 * PI + angleB)));
		
		double theta,delta;
		
		if(dist1 <= dist2){
			theta = dist1 / 2.0;
			delta = -((angleA < angleB ? angleA : angleB) + theta);
		}else{
			theta = dist2 / 2.0;
			delta = -((angleA > angleB ? angleA : angleB) + theta);
		}
		
		// Parameter calculation
		double distA = sqrt(geom::distSquared_D(vw::norm::toD_u(srcX),vw::norm::toD_u(srcY),vw::norm::toD_u(aX),vw::norm::toD_u(aY))) * vw::norm::getZoomScale();
		double distB = sqrt(geom::distSquared_D(vw::norm::toD_u(srcX),vw::norm::toD_u(srcY),vw::norm::toD_u(bX),vw::norm::toD_u(bY))) * vw::norm::getZoomScale();
		
		double rad = distA < radius ? distA : radius;
		rad = distB < rad ? distB : rad;
		
		double a = rad * fabs(cos(theta));
		// double b = rad * fabs((1 - cos(theta)));
		double c = rad * fabs(sin(theta));
		
		// Drawing cone
		if(!outline){
			wedgeCone.setScale(a,c);
			wedgeCone.setRotation(delta * 180.0 / PI);
			wedgeCone.setFillColor(sf::Color(color));
			
			target->draw(
				wedgeCone,
				sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(vw::norm::toD_u(srcX),vw::norm::toD_u(srcY))))
			);
		}
		
		// Drawing cap
		if(outline){
			wedgeCap.setScale(a,c);
			wedgeCap.setRotation(delta * 180.0 / PI);
			wedgeCap.setFillColor(sf::Color(color));
			
			target->draw(
				wedgeCap,
				sf::RenderStates(sf::Transform().translate(
					vw::norm::transform().transformPoint(vw::norm::toD_u(srcX),vw::norm::toD_u(srcY))
				))
			);
		}
	}
	
	void drawMark(unsigned int i,int32_t x,int32_t y,bool isScaleNorm,float scale){
		float newScale = isScaleNorm ? vw::norm::getScale() * scale : scale;
		sf::Shape *drawn = marks[markShape(i)];
		
		drawn->setScale(newScale,newScale);
		drawn->setFillColor(sf::Color(markColor(i,clr::ALF_ONE)));
		
		target->draw(
			*drawn,
			sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(vw::norm::toD_u(x),vw::norm::toD_u(y))))
		);
	}
	
	void drawStem(unsigned int i,int32_t srcX,int32_t srcY,int32_t destX,int32_t destY){
		double srcX_D = vw::norm::toD_u(srcX);
		double srcY_D = vw::norm::toD_u(srcY);
		double destX_D = vw::norm::toD_u(destX);
		double destY_D = vw::norm::toD_u(destY);
		
		float len = sqrt(geom::distSquared_D(srcX_D,srcY_D,destX_D,destY_D)) * vw::norm::getZoomScale();
		float angle = atan2(srcY_D - destY_D,destX_D - srcX_D); // Flipped Y to adjust to SFML coordinates
		
		stem.setRotation(angle * 180.0 / PI);
		stem.setScale(len,1.0);
		stem.setFillColor(sf::Color(markColor(i,clr::ALF_HALF)));
		
		target->draw(
			stem,
			sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(srcX_D,srcY_D)))
		);
	}
	
	void drawLine(enum lineType type,int16_t x,int16_t y,float len,uint32_t color){
		sf::RectangleShape *rect;
		
		switch(type){
			case LINE_VERTICAL:
				vLine.setScale(1.0,len);
				rect = &vLine;
				
				break;
			case LINE_HORIZONTAL:
				hLine.setScale(len,1.0);
				rect = &hLine;
				
				break;
		}
		
		rect->setFillColor(sf::Color(color));
		
		target->draw(
			*rect,
			sf::RenderStates(sf::Transform().translate(vw::norm::transform().transformPoint(vw::norm::toD(x),vw::norm::toD(y))))
		);
	}
	
	namespace ref{
		class loadableTexture refTex;
		
		bool load(const char *source){
			return refTex.load(source,true);
		}
		
		void unload(){
			refTex.unload();
		}
		
		void toggleSmooth(){
			refTex.toggleSmooth();
		}
		
		void draw(){
			refTex.draw();
		}
	}
}