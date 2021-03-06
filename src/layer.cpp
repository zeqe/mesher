#include <climits>
#include <cmath>
#include <cstring>
#include <cfloat>

#include "view.hpp"
#include "layer.hpp"
#include "geometry.hpp"
#include "colors.hpp"
#include "colorsCustom.hpp"
#include "graphics.hpp"
#include "skeleton.hpp"
#include "fileIO.hpp"

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Layer ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
layer::layer(){
	nameSet("");
}

layer::~layer(){
	
}

void layer::visibilityToggle(){
	isVisible = !isVisible;
}

bool layer::visible(){
	return isVisible;
}

void layer::nameSet(const char *newName){
	strncpy(name,newName,LAYER_NAME_STRLEN);
	name[LAYER_NAME_STRLEN] = '\0';
}

const char *layer::nameGet(){
	return name;
}

class layer *layer::withNearestPoint(class layer *layer1,class layer *layer2){
	int l1 = (layer1 != NULL && layer1->visible() && layer1->nearestPoint_Found());
	int l2 = (layer2 != NULL && layer2->visible() && layer2->nearestPoint_Found());
	
	switch((l2 << 1) | l1){
		case 0x00:
			return NULL;
		case 0x01:
			return layer1;
		case 0x02:
			return layer2;
		case 0x03:
			{
				int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
				int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
				
				uint64_t dist1 = geom::distSquared_I(layer1->nearestPoint_X(),layer1->nearestPoint_Y(),iX,iY);
				uint64_t dist2 = geom::distSquared_I(layer2->nearestPoint_X(),layer2->nearestPoint_Y(),iX,iY);
				
				if(dist1 < dist2){
					return layer1;
				}
				
				return layer2;
			}
		default:
			break;
	}
	
	return NULL;
}

class layer *layer::withNearestPoint(std::vector<class layer *> &layers){
	int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
	int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
	
	uint64_t distCurr,distNearest = UINT64_MAX;
	class layer *nearestLayer = NULL;
	
	for(std::vector<class layer *>::iterator it = layers.begin();it != layers.end();++it){
		if((*it) == NULL || !(*it)->visible() || !(*it)->nearestPoint_Found()){
			continue;
		}
		
		distCurr = geom::distSquared_I((*it)->nearestPoint_X(),(*it)->nearestPoint_Y(),iX,iY);
		
		if(distCurr < distNearest){
			distNearest = distCurr;
			nearestLayer = *it;
		}
	}
	
	return nearestLayer;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Vert Layer ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Macros and Pseudo-Macros -------------------------------------------------------------------------------------------------------------------------------------------
#define TRI_V(t,i) ((t) * TRI_VERT_COUNT + (i))

#define NO_NEAR_ELMNT (UINT_MAX)

bool vertLayer::NEARVERT_VALID(){
	return nearVert < buffer.count * TRI_VERT_COUNT;
}

bool vertLayer::NEARTRI_VALID(){
	return nearTri < buffer.count;
}

uint16_t norm16_StoU(int16_t val){
	return (uint16_t)((int32_t)val - (int32_t)INT16_MIN);
}

int16_t norm16_UtoS(uint16_t val){
	return (int16_t)((int32_t)val + (int32_t)INT16_MIN);
}

int16_t norm32_bounded16(int32_t val){
	if(val < INT16_MIN){
		return INT16_MIN;
	}else if(val > INT16_MAX){
		return INT16_MAX;
	}
	
	return val;
}

// Utility Methods -------------------------------------------------------------------------------------------------------------------------------------------
bool vertLayer::readMesh(FILE *in){
	bool success = true,iSuccess;
	
	if(!fIO::u16::read(&(buffer.count),in)){
		return false;
	}
	
	buffer.xy = new int16_t[buffer.count * TRI_XY_VALUE_COUNT];
	buffer.uv = new uint16_t[buffer.count * TRI_UV_VALUE_COUNT];
	buffer.tbc = new uint8_t[buffer.count * TRI_TBC_VALUE_COUNT];
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::s16::read(&(VERT_X(&buffer,i)),in) && fIO::s16::read(&(VERT_Y(&buffer,i)),in);
		success = success && iSuccess;
	}
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::u16::read(&(VERT_U(&buffer,i)),in) && fIO::u16::read(&(VERT_V(&buffer,i)),in);
		success = success && iSuccess;
	}
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::u8::read(&(VERT_TYPE(&buffer,i)),in) && fIO::u8::read(&(VERT_BONE(&buffer,i)),in) && fIO::u8::read(&(VERT_COLOR(&buffer,i)),in);
		success = success && iSuccess;
	}
	
	if(!success){
		delete[] buffer.xy;
		delete[] buffer.uv;
		delete[] buffer.tbc;
	}
	
	return success;
}

bool vertLayer::init_ReadLayer(FILE *in){
	if(!readMesh(in)){
		return false;
	}
	
	// Parameters
	maxTris = buffer.count;
	
	// Buffers
	disp.count = buffer.count;
	disp.xy = new int16_t[maxTris * TRI_XY_VALUE_COUNT];
	disp.uv = new uint16_t[maxTris * TRI_UV_VALUE_COUNT];
	disp.tbc = new uint8_t[maxTris * TRI_TBC_VALUE_COUNT];
	
	modified = true;
	
	// Selections
	bool success = true;
	selVerts = new unsigned char[maxTris * TRI_VERT_COUNT];
	
	selVertCount = 0;
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		success = success && fIO::u8::read(selVerts + i,in);
		
		if(success && selVerts[i]){
			++selVertCount;
		}
	}
	
	// Done
	return success;
}

void vertLayer::init_Blank(unsigned int maxTriCount){
	// Parameters
	maxTris = maxTriCount;
	
	// Buffers
	buffer.count = 0;
	buffer.xy = new int16_t[maxTris * TRI_XY_VALUE_COUNT];
	buffer.uv = new uint16_t[maxTris * TRI_UV_VALUE_COUNT];
	buffer.tbc = new uint8_t[maxTris * TRI_TBC_VALUE_COUNT];
	
	disp.count = 0;
	disp.xy = new int16_t[maxTris * TRI_XY_VALUE_COUNT];
	disp.uv = new uint16_t[maxTris * TRI_UV_VALUE_COUNT];
	disp.tbc = new uint8_t[maxTris * TRI_TBC_VALUE_COUNT];
	
	modified = false;
	
	// Selections
	selVerts = new unsigned char[maxTris * TRI_VERT_COUNT];
	memset(selVerts,0,maxTris * TRI_VERT_COUNT * sizeof(unsigned char));
	
	selVertCount = 0;
}

void vertLayer::end(){
	// Buffers
	delete[] buffer.xy;
	delete[] buffer.uv;
	delete[] buffer.tbc;
	
	delete[] disp.xy;
	delete[] disp.uv;
	delete[] disp.tbc;
	
	deleteVecTris(dispTris);
	
	// Selections
	delete[] selVerts;
}

void vertLayer::copyTri(struct vecTrisBuf *src,unsigned int srcI,struct vecTrisBuf *dest,unsigned int destI){
	memcpy(dest->xy + destI * TRI_XY_VALUE_COUNT,src->xy + srcI * TRI_XY_VALUE_COUNT,TRI_XY_VALUE_COUNT * sizeof(int16_t));
	memcpy(dest->uv + destI * TRI_UV_VALUE_COUNT,src->uv + srcI * TRI_UV_VALUE_COUNT,TRI_UV_VALUE_COUNT * sizeof(uint16_t));
	memcpy(dest->tbc + destI * TRI_TBC_VALUE_COUNT,src->tbc + srcI * TRI_TBC_VALUE_COUNT,TRI_TBC_VALUE_COUNT * sizeof(uint8_t));
}

unsigned int vertLayer::renderVertMode(){
	switch(state::getDraw()){
		case D_STATE_XY:
		case D_STATE_COLOR:
		case D_STATE_BONE:
			return VERT_MODE_RAW_XY;
		case D_STATE_UV:
			return VERT_MODE_RAW_UV;
		case D_STATE_VIEW:
		case D_STATE_POSE:
			return VERT_MODE_POSE_XY;
	}
	
	return VERT_MODE_RAW_XY;
}

unsigned int vertLayer::renderFragMode(){
	switch(state::getDraw()){
		case D_STATE_XY:
		case D_STATE_BONE:
			return FRAG_MODE_CLIPPED_CLR;
		case D_STATE_UV:
		case D_STATE_COLOR:
		case D_STATE_VIEW:
		case D_STATE_POSE:
			return render::tex::isLoaded() ? FRAG_MODE_CLIPPED_CLR_SMPL : FRAG_MODE_CLIPPED_CLR;
	}
	
	return FRAG_MODE_CLIPPED_CLR;
}

unsigned int vertLayer::renderClrPfl(){
	switch(state::getDraw()){
		case D_STATE_XY:
		case D_STATE_UV:
			return CLR_PFL_EDITOR;
		case D_STATE_COLOR:
		case D_STATE_VIEW:
			return CLR_PFL_CSTM;
		case D_STATE_BONE:
		case D_STATE_POSE:
			return CLR_PFL_RANBW;
	}
	
	return CLR_PFL_EDITOR;
}

sf::Vector2<int32_t> vertLayer::modedVertPosition(unsigned int i){
	sf::Vector2<int32_t> vP;
	
	switch(renderVertMode()){
		case VERT_MODE_RAW_UV:
			vP.x = norm16_UtoS(VERT_U(&buffer,i));
			vP.y = norm16_UtoS(VERT_V(&buffer,i));
			
			break;
		case VERT_MODE_POSE_XY:
			vP = pose::getPointPosition(VERT_BONE(&buffer,i),VERT_X(&buffer,i),VERT_Y(&buffer,i));
			
			break;
		case VERT_MODE_RAW_XY:
		default:
			vP.x = VERT_X(&buffer,i);
			vP.y = VERT_Y(&buffer,i);
			
			break;
	}
	
	return vP;
}

void vertLayer::vertModifiers_ApplyTo(struct vecTrisBuf *vertModified){
	if(!vertModifiers_Applicable()){
		return;
	}
	
	// Apply modifier according to global state
	switch(state::get()){
		case STATE_ATOP_TRANSFORM_XY:
			for(unsigned int i = 0;i < vertModified->count * TRI_VERT_COUNT;++i){
				if(selVerts[i]){
					(*vertModifier)(&(VERT_X(vertModified,i)),&(VERT_Y(vertModified,i)));
				}
			}
			
			break;
		case STATE_ATOP_TRANSFORM_UV:
			for(unsigned int i = 0;i < vertModified->count * TRI_VERT_COUNT;++i){
				if(selVerts[i]){
					int16_t nU = norm16_UtoS(VERT_U(vertModified,i));
					int16_t nV = norm16_UtoS(VERT_V(vertModified,i));
					
					(*vertModifier)(&nU,&nV);
					
					VERT_U(vertModified,i) = norm16_StoU(nU);
					VERT_V(vertModified,i) = norm16_StoU(nV);
				}
			}
			
			break;
		default:
			break;
	}
}

// General Globals -------------------------------------------------------------------------------------------------------------------------------------------
vertLayer::vertLayer(unsigned int maxTriCount,FILE *in,bool *readSuccess){
	// Default States ---------------------------------------------
	// Buffer display
	dispTris = NULL;
	
	// Draw State Tracking
	lastDraw = state::getDraw();
	lastBone = 0;
	
	// Vertex Modifiers
	vertModifier = NULL;
	vertModifierEnabled = NULL;
	
	// Near elements
	nearVert = NO_NEAR_ELMNT;
	nearTri = NO_NEAR_ELMNT;
	
	// Buffer & Selection -----------------------------------------
	if(in == NULL || readSuccess == NULL || !(*readSuccess = init_ReadLayer(in))){
		end();
		init_Blank(maxTriCount);
	}
}

vertLayer::~vertLayer(){
	end();
}

enum layerType vertLayer::type(){
	return LAYER_VERT;
}

// Output ---------------------------------------------------------------------------------------------------------------------------------------------------------
bool vertLayer::writeMesh(FILE *out){
	bool success = true,iSuccess;
	
	if(!fIO::u16::write(buffer.count,out)){
		return false;
	}
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::s16::write(VERT_X(&buffer,i),out) && fIO::s16::write(VERT_Y(&buffer,i),out);
		success = success && iSuccess;
	}
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::u16::write(VERT_U(&buffer,i),out) && fIO::u16::write(VERT_V(&buffer,i),out);
		success = success && iSuccess;
	}
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		iSuccess = fIO::u8::write(VERT_TYPE(&buffer,i),out) && fIO::u8::write(VERT_BONE(&buffer,i),out) && fIO::u8::write(VERT_COLOR(&buffer,i),out);
		success = success && iSuccess;
	}
	
	return success;
}

bool vertLayer::writeLayer(FILE *out){
	// Buffer
	if(!writeMesh(out)){
		return false;
	}
	
	// Selection
	bool success = true;
	
	for(unsigned int i = 0;success && i < buffer.count * TRI_VERT_COUNT;++i){
		success = success && fIO::u8::write(selVerts[i],out);
	}
	
	return success;
}


// Vertex Modifiers -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::vertModifiers_Set(void (*mod)(int16_t*,int16_t*),bool (*modEnabled)()){
	vertModifier = mod;
	vertModifierEnabled = modEnabled;
}

bool vertLayer::vertModifiers_Applicable(){
	return vertModifier != NULL && vertModifierEnabled != NULL && visible() && (*vertModifierEnabled)() && selVertCount > 0;
}

void vertLayer::vertModifiers_Apply(){
	vertModifiers_ApplyTo(&buffer);
}

// Inherited -------------------------------------------------------------------------------------------------------------------------------------------
bool vertLayer::nearestPoint_Find(unsigned int radius){
	return nearestPoint_Find(radius,0);
}

bool vertLayer::nearestPoint_Find(unsigned int radius,unsigned char currBone){
	nearestPoint_Clear();
	
	if(!visible()){
		return false;
	}
	
	int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
	int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
	
	// Searching for nearest vertex index
	uint64_t dist,nearDist = UINT64_MAX;
	sf::Vector2<int32_t> vPs[TRI_VERT_COUNT];
	
	for(unsigned int i = 0;i < buffer.count;++i){
		// Calculate triangle vertex positions
		for(unsigned int j = 0;j < TRI_VERT_COUNT;++j){
			vPs[j] = modedVertPosition(TRI_V(i,j));
		}
		
		// Perform closest-vertex search
		for(unsigned int j = 0;j < TRI_VERT_COUNT;++j){
			// Skip vertices amid modification
			if(
				(*vertModifierEnabled)() && (
					(renderVertMode() != VERT_MODE_POSE_XY && vertModifiers_Applicable() && selVerts[i]) ||
					(renderVertMode() == VERT_MODE_POSE_XY && VERT_BONE(&buffer,TRI_V(i,j)) == currBone)
				)
			){
				continue;
			}
			
			// Considering only those within the radius and hovered triangle
			dist = geom::distSquared_I(vPs[j].x,vPs[j].y,iX,iY);
			
			if(
				dist < radius * radius && dist < nearDist &&
				geom::pointInTri(
					iX,iY,
					vPs[0].x,vPs[0].y,
					vPs[1].x,vPs[1].y,
					vPs[2].x,vPs[2].y
				)
			){
				// Current elements
				nearVert = TRI_V(i,j);
				nearTri = i;
				
				// Neighbor data
				for(unsigned int k = 0;k < TRI_VERT_COUNT;++k){
					neighborVerts[k * 2 + 0] = vPs[k].x;
					neighborVerts[k * 2 + 1] = vPs[k].y;
				}
				
				neighborCurrent = j;
				
				// Algorithm metric update
				nearDist = dist;
			}
		}
	}
	
	// Returning status
	return nearVert != NO_NEAR_ELMNT;
}

bool vertLayer::nearestPoint_Found(){
	return nearVert != NO_NEAR_ELMNT;
}

void vertLayer::nearestPoint_Clear(){
	nearVert = NO_NEAR_ELMNT;
	nearTri = NO_NEAR_ELMNT;
}

int16_t vertLayer::nearestPoint_X(){
	return norm32_bounded16(neighborVerts[neighborCurrent * 2 + 0]);
}

int16_t vertLayer::nearestPoint_Y(){
	return norm32_bounded16(neighborVerts[neighborCurrent * 2 + 1]);
}

class vertLayer *vertLayer::withNearestPoint(std::vector<class vertLayer *> &layers){
	int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
	int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
	
	uint64_t distCurr,distNearest = UINT64_MAX;
	class vertLayer *nearestLayer = NULL;
	
	for(std::vector<class vertLayer *>::iterator it = layers.begin();it != layers.end();++it){
		if((*it) == NULL || !(*it)->visible() || !(*it)->nearestPoint_Found()){
			continue;
		}
		
		distCurr = geom::distSquared_I((*it)->nearestPoint_X(),(*it)->nearestPoint_Y(),iX,iY);
		
		if(distCurr < distNearest){
			distNearest = distCurr;
			nearestLayer = *it;
		}
	}
	
	return nearestLayer;
}

// Drawing -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::draw(){
	draw(0,false,false);
}

void vertLayer::draw(unsigned char currBone,bool wireframe,bool showNearestPoint){
	if(!visible()){
		return;
	}
	
	// State track updasion
	if(state::getDraw() != lastDraw || currBone != lastBone){
		lastDraw = state::getDraw();
		lastBone = currBone;
		
		modified = true;
	}
	
	// Draw buffer -------------------------------------------	
	// Display tris updasion and rendering
	if(modified || vertModifiers_Applicable()){
		// Copying to display buffer
		disp.count = buffer.count;
		memcpy(disp.xy,buffer.xy,buffer.count * TRI_XY_VALUE_COUNT * sizeof(int16_t));
		memcpy(disp.uv,buffer.uv,buffer.count * TRI_UV_VALUE_COUNT * sizeof(uint16_t));
		memcpy(disp.tbc,buffer.tbc,buffer.count * TRI_TBC_VALUE_COUNT * sizeof(uint8_t));
		
		// Applying modifiers if needed
		if(vertModifiers_Applicable()){
			vertModifiers_ApplyTo(&disp);
		}
		
		// Set vertex colors if applicable
		if(renderClrPfl() == CLR_PFL_EDITOR){
			for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
				VERT_COLOR(&disp,i) = selVerts[i] ? CLR_EDITR_HILIGHT : CLR_EDITR_OFFWHITE;
			}
		}else if(renderClrPfl() == CLR_PFL_RANBW){
			for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
				VERT_COLOR(&disp,i) = (VERT_BONE(&buffer,i) == currBone ? hud::markColorI(VERT_BONE(&buffer,i)) : CLR_RANBW_NULL);
			}
		}
		
		// Render
		render::loadAndDrawTris(&disp,&dispTris,renderVertMode(),renderFragMode(),renderClrPfl(),wireframe);
	}else{
		// Default rendering
		render::loadAndDrawTris(NULL,&dispTris,renderVertMode(),renderFragMode(),renderClrPfl(),wireframe);
	}
	
	// State finalization
	modified = vertModifiers_Applicable();
	
	// None-pose indicators ahead -----------------------------------------------------------------------------------------------------------------------------------
	if(renderVertMode() == VERT_MODE_POSE_XY){
		return;
	}
	
	// Draw nearest vertex/currrent triangle indicator -------------------------------------------
	if(showNearestPoint && nearVert != NO_NEAR_ELMNT){
		// Color calculation
		uint32_t indicatorColor;
		
		switch(renderClrPfl()){
			case CLR_PFL_CSTM:
				indicatorColor = (clr::inverse(clrCstm::get(VERT_COLOR(&disp,nearVert))) & 0xffffff00) | clr::getAlpha(clr::ALF_HALF);
				
				break;
			case CLR_PFL_RANBW:
				indicatorColor = clr::inverse(clr::get(clr::PFL_RANBW,VERT_COLOR(&disp,nearVert),clr::ALF_HALF));
				
				break;
			case CLR_PFL_EDITOR:
			default:
				indicatorColor = clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_HALF);
				
				break;
		}
		
		// Indices calculation
		unsigned int a = neighborCurrent;
		unsigned int b = (neighborCurrent + 1) % TRI_VERT_COUNT;
		unsigned int c = (neighborCurrent + 2) % TRI_VERT_COUNT;
		
		// Draw
		hud::drawWedge(
			neighborVerts[a * 2 + 0],neighborVerts[a * 2 + 1],
			neighborVerts[b * 2 + 0],neighborVerts[b * 2 + 1],
			neighborVerts[c * 2 + 0],neighborVerts[c * 2 + 1],
			TRI_VERT_MARKER_RADIUS,indicatorColor
		);
	}
	
	// Draw convex handles -------------------------------------------
	sf::Vector2<int16_t> vP16;
	sf::Vector2<int32_t> vP32;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(VERT_TYPE(&buffer,i) != TRI_TYPE_CONVEX || (i % 3) != 0){
			continue;
		}
		
		// Vertex position retrieval and adjustment as needed
		vP32 = modedVertPosition(i);
		vP16.x = vP32.x;
		vP16.y = vP32.y;
		
		if(vertModifiers_Applicable() && selVerts[i]){
			(*vertModifier)(&(vP16.x),&(vP16.y));
		}
		
		// Drawing
		hud::drawCircle(vP16.x,vP16.y,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,selVerts[i] ? CLR_EDITR_HILIGHT : CLR_EDITR_OFFWHITE,clr::ALF_HALF));
	}
}

// Selections -------------------------------------------------------------------------------------------------------------------------------------------
bool vertLayer::selectVert_Nearest(bool toggle,bool set){
	if(!NEARVERT_VALID()){
		return set;
	}
	
	bool newState = toggle ? !selVerts[nearVert] : set;
	
	selVertCount += selVerts[nearVert] == newState ? 0 : ((char)newState * 2 - 1);
	selVerts[nearVert] = newState;
	
	modified = true;
	
	return newState;
}

void vertLayer::selectVert_All(){
	memset(selVerts,1,buffer.count * TRI_VERT_COUNT * sizeof(unsigned char));
	selVertCount = buffer.count * TRI_VERT_COUNT;
	
	modified = true;
}

void vertLayer::selectVert_Clear(){
	memset(selVerts,0,maxTris * TRI_VERT_COUNT * sizeof(unsigned char));
	selVertCount = 0;
	
	modified = true;
}

// Buffer Operations -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::nearVert_SetColor(unsigned char color){
	if(!NEARVERT_VALID()){
		return;
	}
	
	VERT_COLOR(&buffer,nearVert) = color;
	modified = true;
}

void vertLayer::nearVert_SetBone(unsigned char bone){
	if(!NEARVERT_VALID()){
		return;
	}
	
	VERT_BONE(&buffer,nearVert) = bone;
	modified = true;
}

void vertLayer::nearTri_Delete(){
	if(!NEARTRI_VALID()){
		return;
	}
	
	// Copying triangle from the end to nearest triangle, effectively replacing it
	--buffer.count;
	
	copyTri(&buffer,buffer.count,&buffer,nearTri);
	
	// Updating vertex selections
	for(unsigned int i = 0;i < TRI_VERT_COUNT;++i){
		if(selVerts[TRI_V(nearTri,i)]){
			--selVertCount;
		}
		
		selVerts[TRI_V(nearTri,i)] = selVerts[TRI_V(buffer.count,i)];
	}
	
	// Updating state
	modified = true;
	
	// Copying over the last triangle may have invalidated indices
	nearestPoint_Clear();
}

void vertLayer::tris_Add(int16_t x0,int16_t y0,int16_t x1,int16_t y1,int16_t x2,int16_t y2,unsigned char type){
	if(buffer.count >= maxTris){
		return;
	}
	
	// Coordinates
	VERT_X(&buffer,TRI_V(buffer.count,0)) = x0;
	VERT_Y(&buffer,TRI_V(buffer.count,0)) = y0;
	
	VERT_X(&buffer,TRI_V(buffer.count,1)) = x1;
	VERT_Y(&buffer,TRI_V(buffer.count,1)) = y1;
	
	VERT_X(&buffer,TRI_V(buffer.count,2)) = x2;
	VERT_Y(&buffer,TRI_V(buffer.count,2)) = y2;
	
	// Other uniform data
	for(unsigned int i = 0;i < TRI_VERT_COUNT;++i){
		unsigned int j = TRI_V(buffer.count,i);
		
		VERT_U(&buffer,j) = norm16_StoU(VERT_X(&buffer,j));
		VERT_V(&buffer,j) = norm16_StoU(VERT_Y(&buffer,j));
		
		VERT_TYPE(&buffer,j) = type;
		VERT_BONE(&buffer,j) = 0;
		VERT_COLOR(&buffer,j) = 0;
	}
	
	// Selection clearance
	selVerts[TRI_V(buffer.count,0)] = 0;
	selVerts[TRI_V(buffer.count,1)] = 0;
	selVerts[TRI_V(buffer.count,2)] = 0;
	
	// Update state
	++buffer.count;
	modified = true;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Grid Layer ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Generals -------------------------------------------------------------------------------------------------------------------------------------------
gridLayer::gridLayer(){
	
}

gridLayer::~gridLayer(){
	
}

void gridLayer::set(unsigned char divs){
	divisions = divs;
}

unsigned char gridLayer::get(){
	return divisions;
}

enum layerType gridLayer::type(){
	return LAYER_GRID;
}

// Inherited -------------------------------------------------------------------------------------------------------------------------------------------
bool gridLayer::nearestPoint_Find(unsigned int radius){
	// Search for nearest grid joint
	nearestPoint_Clear();
	
	if(!visible()){
		return false;
	}
	
	int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
	int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
	
	uint64_t dist,nearDist = UINT64_MAX;
	
	// Calculate grid distances
	double block = 2.0 / ((double)divisions + 1.0);
	
	double uX = vw::norm::cursorPos().x + 1.0;
	double uY = vw::norm::cursorPos().y + 1.0;
	
	double nearAxis[4];
	nearAxis[0] = floor(uX / block);
	nearAxis[1] = floor(uY / block);
	nearAxis[2] = ceil(uX / block);
	nearAxis[3] = ceil(uY / block);
	
	// Search for nearest grid node
	int16_t gX,gY;
	
	for(unsigned int i = 0;i < 4;++i){
		gX = vw::norm::toI(-1.0 + nearAxis[(i / 2) * 2] * block);
		gY = vw::norm::toI(-1.0 + nearAxis[1 + (i % 2) * 2] * block);
		
		dist = geom::distSquared_I(gX,gY,iX,iY);
		
		if(dist < radius * radius && dist < nearDist){
			nearDist = dist;
			
			isNear = true;
			nearX = gX;
			nearY = gY;
		}
	}
	
	return isNear;
}

bool gridLayer::nearestPoint_Found(){
	return isNear;
}

void gridLayer::nearestPoint_Clear(){
	isNear = false;
}

int16_t gridLayer::nearestPoint_X(){
	return nearX;
}

int16_t gridLayer::nearestPoint_Y(){
	return nearY;
}

void gridLayer::draw(){
	if(!visible()){
		return;
	}
	
	double unit = 2.0 / ((double)divisions + 1.0);
	
	for(unsigned int i = 0;i < (unsigned int)divisions + 2;++i){
		hud::drawLine(hud::LINE_VERTICAL,vw::norm::toI(-1.0 + unit * (double)i),0,vw::norm::getZoomScale(),clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_BUTL));
		hud::drawLine(hud::LINE_HORIZONTAL,0,vw::norm::toI(-1.0 + unit * (double)i),vw::norm::getZoomScale(),clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_BUTL));
	}
}