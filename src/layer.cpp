#include <climits>
#include <cmath>
#include <cstring>
#include <cfloat>

#include <iostream>

#include "view.hpp"
#include "layer.hpp"
#include "geometry.hpp"
#include "colors.hpp"
#include "graphics.hpp"
#include "skeleton.hpp"

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

unsigned int vertLayer::BUFF_XY_COUNT(){
	return maxTris * TRI_XY_VALUE_COUNT;
}

unsigned int vertLayer::BUFF_UV_COUNT(){
	return maxTris * TRI_UV_VALUE_COUNT;
}

unsigned int vertLayer::BUFF_TBC_COUNT(){
	return maxTris * TRI_TBC_VALUE_COUNT;
}

bool vertLayer::NEARVERT_VALID(){
	return nearVert < buffer.count * TRI_VERT_COUNT;
}

bool vertLayer::NEARTRI_VALID(){
	return nearTri < buffer.count;
}

unsigned int vertLayer::SEL_VERTS_COUNT(){
	return maxTris * TRI_VERT_COUNT;
}

unsigned int vertLayer::SEL_TRIS_COUNT(){
	return maxTris;
}

// Utility Methods -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::init(unsigned int maxTriCount,enum viewType initView){
	// Parameters
	maxTris = maxTriCount;
	
	// Buffers
	buffer.count = 0;
	buffer.xy = new int16_t[BUFF_XY_COUNT()];
	buffer.uv = new uint16_t[BUFF_UV_COUNT()];
	buffer.tbc = new uint8_t[BUFF_TBC_COUNT()];
	
	disp.count = 0;
	disp.xy = new int16_t[BUFF_XY_COUNT()];
	disp.uv = new uint16_t[BUFF_UV_COUNT()];
	disp.tbc = new uint8_t[BUFF_TBC_COUNT()];
	
	dispTris = NULL;
	
	modified = false;
	
	// View State
	currentView = initView;
	
	// Vertex Modifiers
	vertModifier = NULL;
	vertModifierEnabled = NULL;
	
	// Near elements
	nearVert = NO_NEAR_ELMNT;
	nearTri = NO_NEAR_ELMNT;
	
	// Selections
	selVerts = new unsigned char[SEL_VERTS_COUNT()];
	memset(selVerts,0,SEL_VERTS_COUNT() * sizeof(unsigned char));
	
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

bool vertLayer::vertsWelded(unsigned int i,unsigned int j){
	return VERT_X(&buffer,i) == VERT_X(&buffer,j) && VERT_Y(&buffer,i) == VERT_Y(&buffer,j);
}

void vertLayer::copyVertSharedAttribs(unsigned int srcI,unsigned int destI){
	VERT_U(&buffer,destI) = VERT_U(&buffer,srcI);
	VERT_V(&buffer,destI) = VERT_V(&buffer,srcI);
	
	VERT_BONE(&buffer,destI) = VERT_BONE(&buffer,srcI);
}

void vertLayer::copyTri(struct vecTrisBuf *src,unsigned int srcI,struct vecTrisBuf *dest,unsigned int destI){
	memcpy(dest->xy + destI * TRI_XY_VALUE_COUNT,src->xy + srcI * TRI_XY_VALUE_COUNT,TRI_XY_VALUE_COUNT * sizeof(int16_t));
	memcpy(dest->uv + destI * TRI_UV_VALUE_COUNT,src->uv + srcI * TRI_UV_VALUE_COUNT,TRI_UV_VALUE_COUNT * sizeof(uint16_t));
	memcpy(dest->tbc + destI * TRI_TBC_VALUE_COUNT,src->tbc + srcI * TRI_TBC_VALUE_COUNT,TRI_TBC_VALUE_COUNT * sizeof(uint8_t));
}

// General Globals -------------------------------------------------------------------------------------------------------------------------------------------
vertLayer::vertLayer(unsigned int maxTriCount,enum viewType initView){
	init(maxTriCount,initView);
}

vertLayer::~vertLayer(){
	end();
}

enum layerType vertLayer::type(){
	return LAYER_VERT;
}

// View State -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::setView(enum viewType newView){
	if(newView != currentView && (newView == VIEW_COLORS || currentView == VIEW_COLORS)){
		modified = true;
	}
	
	currentView = newView;
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
	if(!vertModifiers_Applicable()){
		return;
	}
	
	// Apply modifier
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(selVerts[i]){
			(*vertModifier)(&(VERT_X(&buffer,i)),&(VERT_Y(&buffer,i)));
		}
	}
	
	// Update selections for seamless selection post-weld
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(selVerts[i]){
			for(unsigned int j = 0;j < buffer.count * TRI_VERT_COUNT;++j){
				if(!selVerts[j] && vertsWelded(i,j)){
					copyVertSharedAttribs(i,j);
					selVerts[j] = true;
				}
			}
		}
	}
	
	// (n * s) * (n * (1 - s)) = n^2 * (1 - s)s = n^2 * (s - s^2)
}

// Inherited -------------------------------------------------------------------------------------------------------------------------------------------
bool vertLayer::nearestPoint_Find(unsigned int radius){
	nearestPoint_Clear();
	
	if(!visible()){
		return false;
	}
	
	int16_t iX = vw::norm::toI(vw::norm::cursorPos().x);
	int16_t iY = vw::norm::toI(vw::norm::cursorPos().y);
	
	// Searching for nearest vertex index
	uint64_t dist,nearDist = UINT64_MAX;
	unsigned int tri;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		// Skip selected vertices amid modification
		if(vertModifiers_Applicable() && selVerts[i]){
			continue;
		}
		
		// Considering only those within the radius and hovered triangle
		dist = geom::distSquared_I(VERT_X(&buffer,i),VERT_Y(&buffer,i),iX,iY);
		tri = i / TRI_VERT_COUNT;
		
		if(
			dist < radius * radius && dist < nearDist &&
			geom::pointInTri(
				iX,iY,
				VERT_X(&buffer,TRI_V(tri,0)),VERT_Y(&buffer,TRI_V(tri,0)),
				VERT_X(&buffer,TRI_V(tri,1)),VERT_Y(&buffer,TRI_V(tri,1)),
				VERT_X(&buffer,TRI_V(tri,2)),VERT_Y(&buffer,TRI_V(tri,2))
			)
		){
			nearVert = i;
			nearTri = tri;
			
			nearDist = dist;
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
	return VERT_X(&buffer,nearVert);
}

int16_t vertLayer::nearestPoint_Y(){
	return VERT_Y(&buffer,nearVert);
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
	draw(false,false);
}

void vertLayer::draw(bool wireframe,bool showNearestPoint){
	if(!visible()){
		return;
	}
	
	// Draw buffer -------------------------------------------
	enum render::mode drawMode;
	
	switch(currentView){
		case VIEW_XY:
		case VIEW_COLORS:
		case VIEW_BONES:
			drawMode = render::MODE_XY;
			
			break;
		case VIEW_UV:
			drawMode = render::MODE_UV;
			
			break;
		case VIEW_POSE:
			drawMode = render::MODE_POSE;
			
			break;
	}
	
	// Display tris updasion and rendering
	if(modified || vertModifiers_Applicable()){
		// Copying to display buffer
		disp.count = buffer.count;
		memcpy(disp.xy,buffer.xy,buffer.count * TRI_XY_VALUE_COUNT * sizeof(int16_t));
		memcpy(disp.uv,buffer.uv,buffer.count * TRI_UV_VALUE_COUNT * sizeof(uint16_t));
		memcpy(disp.tbc,buffer.tbc,buffer.count * TRI_TBC_VALUE_COUNT * sizeof(uint8_t));
		
		// Applying modifiers if needed
		if(vertModifiers_Applicable()){
			for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
				if(selVerts[i]){
					(*vertModifier)(&(VERT_X(&disp,i)),&(VERT_Y(&disp,i)));
				}
			}
		}
		
		// Set vertex colors if applicable
		/*if(currentView != VIEW_COLORS){
			for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
				VERT_COLOR(&disp,i) = selVerts[i] ? CLR_EDITR_HILIGHT : CLR_EDITR_OFFWHITE;
			}
		}*/
		
		// Render
		render::loadAndDrawTris(&disp,&dispTris,drawMode,currentView == VIEW_COLORS,wireframe,showNearestPoint ? nearVert : buffer.count * TRI_VERT_COUNT);
	}else{
		// Default rendering
		render::loadAndDrawTris(NULL,&dispTris,drawMode,currentView == VIEW_COLORS,wireframe,showNearestPoint ? nearVert : buffer.count * TRI_VERT_COUNT);
	}
	
	// State finalization
	modified = vertModifiers_Applicable();
	
	// Draw nearest vertex/currrent triangle indicator -------------------------------------------
	if(showNearestPoint && nearVert != NO_NEAR_ELMNT){
		//hud::drawCircle(nearestPoint_X(),nearestPoint_Y(),false,POINT_RADIUS_AURA,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
		
		unsigned int base = nearTri * TRI_VERT_COUNT;
		unsigned int srcMod3 = (nearVert - base);
		
		unsigned int a = base + ((srcMod3 + 1) % TRI_VERT_COUNT);
		unsigned int b = base + ((srcMod3 + 2) % TRI_VERT_COUNT);
		
		hud::drawWedge(VERT_X(&disp,nearVert),VERT_Y(&disp,nearVert),VERT_X(&disp,a),VERT_Y(&disp,a),VERT_X(&disp,b),VERT_Y(&disp,b),!selVerts[nearVert] ? 86.0 : 60.0,selVerts[nearVert],clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_HALF));
	}
	
	// Draw convex handles -------------------------------------------
	int16_t preDrawX,preDrawY;
	int32_t drawX,drawY;
	
	if(currentView == VIEW_XY || currentView == VIEW_UV){
		for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
			if(VERT_TYPE(&buffer,i) != TRI_TYPE_CONVEX || (i % 3) != 0){
				continue;
			}
			
			// Vertex position retrieval and adjustment as needed
			if(currentView == VIEW_UV){
				preDrawX = VERT_U(&buffer,i);
				preDrawY = VERT_V(&buffer,i);
			}else{
				preDrawX = VERT_X(&buffer,i);
				preDrawY = VERT_Y(&buffer,i);
			}
			
			if(selVerts[i] && vertModifiers_Applicable()){
				(*vertModifier)(&preDrawX,&preDrawY);
			}
			
			if(currentView == VIEW_POSE){
				sf::Vector2<int32_t> tPos = pose::getPointPosition(VERT_BONE(&buffer,i),preDrawX,preDrawY);
				
				drawX = tPos.x;
				drawY = tPos.y;
			}else{
				drawX = preDrawX;
				drawY = preDrawY;
			}
			
			// Drawing
			hud::drawCircle(drawX,drawY,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,selVerts[i] ? CLR_EDITR_HILIGHT : CLR_EDITR_OFFWHITE,clr::ALF_HALF));
		}
	}
}

// Selections -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::selectVert_Nearest(){
	if(!NEARVERT_VALID()){
		return;
	}
	
	bool newState = !selVerts[nearVert];
	int inc = (newState) - (!newState);
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(vertsWelded(i,nearVert) && selVerts[i] != newState){
			selVerts[i] = newState;
			selVertCount += inc;
		}
	}
	
	modified = true;
}

void vertLayer::selectVert_All(){
	memset(selVerts,1,buffer.count * TRI_VERT_COUNT * sizeof(unsigned char));
	selVertCount = buffer.count * TRI_VERT_COUNT;
	
	modified = true;
}

void vertLayer::selectVert_Clear(){
	memset(selVerts,0,SEL_VERTS_COUNT() * sizeof(unsigned char));
	selVertCount = 0;
	
	modified = true;
}

void vertLayer::selectVert_ByBone(unsigned char bone){
	selVertCount = 0;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		selVerts[i] = (VERT_BONE(&buffer,i) == bone);
		
		if(selVerts[i]){
			++selVertCount;
		}
	}
	
	modified = true;
}

void vertLayer::selectVert_ByColor(unsigned char color){
	selVertCount = 0;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		selVerts[i] = (VERT_COLOR(&buffer,i) == color);
		
		if(selVerts[i]){
			++selVertCount;
		}
	}
	
	modified = true;
}

// Buffer Operations -------------------------------------------------------------------------------------------------------------------------------------------
uint16_t norm16_StoU(int16_t val){
	return (uint16_t)((int32_t)val - (int32_t)INT16_MIN);
}

int16_t norm16_UtoS(uint16_t val){
	return (int16_t)((int32_t)val + (int32_t)INT16_MIN);
}

void vertLayer::nearVert_SetColor(unsigned char color){
	bool set = false;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(i / TRI_VERT_COUNT == nearTri && vertsWelded(i,nearVert)){
			VERT_COLOR(&buffer,i) = color;
			set = true;
		}
	}
	
	modified = modified || set;
}

void vertLayer::nearVert_SetBone(unsigned char bone){
	bool set = false;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(vertsWelded(i,nearVert)){
			VERT_BONE(&buffer,i) = bone;
			set = true;
		}
	}
	
	modified = modified || set;
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
	unsigned int shared,k;
	
	for(unsigned int i = 0;i < TRI_VERT_COUNT;++i){
		k = TRI_V(buffer.count,i);
		
		// Check for welded vertices upon creation
		shared = buffer.count * TRI_VERT_COUNT;
		
		for(unsigned int j = 0;j < buffer.count * TRI_VERT_COUNT;++j){
			if(vertsWelded(k,j)){
				shared = j;
			}
		}
		
		// Assign attribute values accordingly
		VERT_TYPE(&buffer,k) = type;
		VERT_COLOR(&buffer,k) = 0;
		
		if(shared < buffer.count * TRI_VERT_COUNT){
			// Welded case
			copyVertSharedAttribs(shared,k);
		}else{
			// Unwelded case
			VERT_U(&buffer,k) = norm16_StoU(VERT_X(&buffer,k));
			VERT_V(&buffer,k) = norm16_StoU(VERT_Y(&buffer,k));
			
			VERT_BONE(&buffer,k) = 0;
		}
	}
	
	// Selection clearance
	selVerts[TRI_V(buffer.count,0)] = 0;
	selVerts[TRI_V(buffer.count,1)] = 0;
	selVerts[TRI_V(buffer.count,2)] = 0;
	
	// Update state
	++buffer.count;
	modified = true;
}

/*void vertLayer::tris_DeleteSelected(){
	if(!NEARVERT_VALID()){
		return;
	}
	
	if(!selTris[nearTri]){
		return;
	}
	
	selVertCount = 0;
	
	// Removing selected triangles
	unsigned int curr = 0;
	
	for(unsigned int i = 0;i < buffer.count;++i){
		// Copy over if not selected, essentially skipping selected ones for overwriting / non-counting
		if(!selTris[i]){
			if(i != curr){
				copyTri(&buffer,i,&buffer,curr);
				
				selVerts[TRI_V(curr,0)] = selVerts[TRI_V(i,0)];
				selVerts[TRI_V(curr,1)] = selVerts[TRI_V(i,1)];
				selVerts[TRI_V(curr,2)] = selVerts[TRI_V(i,2)];
				
				selTris[curr] = selTris[i];
				
				selVertCount += (selVerts[TRI_V(curr,0)] > 0) + (selVerts[TRI_V(curr,1)] > 0) + (selVerts[TRI_V(curr,2)] > 0);
			}
			
			++curr;
		}
	}
	
	// Updating state
	buffer.count = curr;
	modified = true;
	
	// Copying over triangles by index will have invalidated old indices
	nearestPoint_Clear();
}*/

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