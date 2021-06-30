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

unsigned int vertLayer::BUFF_XYUV_COUNT(){
	return maxTris * TRI_XYUV_VALUE_COUNT;
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
	buffer.xyuv = new int16_t[BUFF_XYUV_COUNT()];
	buffer.tbc = new uint8_t[BUFF_TBC_COUNT()];
	
	disp.count = 0;
	disp.xyuv = new int16_t[BUFF_XYUV_COUNT()];
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
	selTris = new unsigned char[SEL_TRIS_COUNT()];
	
	memset(selVerts,0,SEL_VERTS_COUNT() * sizeof(unsigned char));
	memset(selTris,0,SEL_TRIS_COUNT() * sizeof(unsigned char));
	
	selVertCount = 0;
}

void vertLayer::end(){
	// Buffers
	delete[] buffer.xyuv;
	delete[] buffer.tbc;
	
	delete[] disp.xyuv;
	delete[] disp.tbc;
	
	deleteVecTris(dispTris);
	
	// Selections
	delete[] selVerts;
	delete[] selTris;
}

void vertLayer::copyTri(struct vecTrisBuf *src,unsigned int srcI,struct vecTrisBuf *dest,unsigned int destI){
	memcpy(dest->xyuv + destI * TRI_XYUV_VALUE_COUNT,src->xyuv + srcI * TRI_XYUV_VALUE_COUNT,TRI_XYUV_VALUE_COUNT * sizeof(int16_t));
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
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(selVerts[i]){
			(*vertModifier)(&(VERT_X(&buffer,i)),&(VERT_Y(&buffer,i)));
		}
	}
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
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		// Skip selected vertices amid modification
		if(vertModifiers_Applicable() && selVerts[i]){
			continue;
		}
		
		// Considering only those within the radius
		dist = geom::distSquared_I(VERT_X(&buffer,i),VERT_Y(&buffer,i),iX,iY);
		
		if(dist < radius * radius && dist < nearDist){
			nearVert = i;
			nearDist = dist;
		}
	}
	
	// Handling unfound near element
	if(nearVert == NO_NEAR_ELMNT){
		return false;
	}
	
	// Locate nearest tri
	unsigned int tri;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		// Must share coordinates with nearest vertex
		if(VERT_X(&buffer,i) != VERT_X(&buffer,nearVert) || VERT_Y(&buffer,i) != VERT_Y(&buffer,nearVert)){
			continue;
		}
		
		tri = i / TRI_VERT_COUNT;
		
		// Is set if point is found inside tri
		if(geom::pointInTri(
			iX,iY,
			VERT_X(&buffer,TRI_V(tri,0)),VERT_Y(&buffer,TRI_V(tri,0)),
			VERT_X(&buffer,TRI_V(tri,1)),VERT_Y(&buffer,TRI_V(tri,1)),
			VERT_X(&buffer,TRI_V(tri,2)),VERT_Y(&buffer,TRI_V(tri,2))
		)){
			// Unconditional setting makes for top-most selection
			nearTri = tri;
		}
	}
	
	return true;
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
	// Display tris updasion and rendering
	if(modified || vertModifiers_Applicable()){
		// Copying to display buffer
		disp.count = buffer.count;
		memcpy(disp.xyuv,buffer.xyuv,buffer.count * TRI_XYUV_VALUE_COUNT * sizeof(uint16_t));
		memcpy(disp.tbc,buffer.tbc,buffer.count * TRI_TBC_VALUE_COUNT * sizeof(uint8_t));
		
		// Applying modifiers if needed
		if(vertModifiers_Applicable()){
			for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
				if(selVerts[i]){
					(*vertModifier)(&(VERT_X(&disp,i)),&(VERT_Y(&disp,i)));
				}
			}
		}
		
		// Setting appropriate triangle colors if needed
		if(currentView != VIEW_COLORS){
			for(unsigned int i = 0;i < buffer.count;++i){
				if(selTris[i]){
					VERT_COLOR(&disp,TRI_V(i,0)) = CLR_EDITR_HILIGHT;
					VERT_COLOR(&disp,TRI_V(i,1)) = CLR_EDITR_HILIGHT;
					VERT_COLOR(&disp,TRI_V(i,2)) = CLR_EDITR_HILIGHT;
				}else{
					VERT_COLOR(&disp,TRI_V(i,0)) = CLR_EDITR_OFFWHITE;
					VERT_COLOR(&disp,TRI_V(i,1)) = CLR_EDITR_OFFWHITE;
					VERT_COLOR(&disp,TRI_V(i,2)) = CLR_EDITR_OFFWHITE;
				}
			}
		}
		
		// Render
		render::loadAndDrawTris(&disp,&dispTris,currentView == VIEW_UV,currentView == VIEW_COLORS,wireframe);
	}else{
		// Default rendering
		render::loadAndDrawTris(NULL,&dispTris,currentView == VIEW_UV,currentView == VIEW_COLORS,wireframe);
	}
	
	// State finalization
	modified = vertModifiers_Applicable();
	
	// Draw nearest vertex -------------------------------------------
	if(showNearestPoint && nearVert != NO_NEAR_ELMNT){
		hud::drawMark(hud::MARK_CIRCLE,nearestPoint_X(),nearestPoint_Y(),false,POINT_RADIUS_AURA,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
	}
	
	// Draw vertices -------------------------------------------
	int16_t drawX,drawY;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		// Vertex position retrieval and adjustment as needed
		if(currentView == VIEW_UV){
			drawX = VERT_U(&buffer,i);
			drawY = VERT_V(&buffer,i);
		}else{
			drawX = VERT_X(&buffer,i);
			drawY = VERT_Y(&buffer,i);
		}
		
		if(selVerts[i] && vertModifiers_Applicable()){
			(*vertModifier)(&drawX,&drawY);
		}
		
		// Drawing
		uint32_t color;
		enum hud::markType marker = hud::MARK_CIRCLE;
		
		switch(currentView){
			case VIEW_XY:
			case VIEW_UV:
				if(selVerts[i]){
					color = clr::get(clr::PFL_EDITR,CLR_EDITR_HILIGHT,clr::ALF_HALF);
					
				}else if(VERT_TYPE(&buffer,i) == TRI_TYPE_CONVEX && (i % 3) == 0){
					color = clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF);
					
				}else{
					continue;
				}
				
				break;
			case VIEW_COLORS:
				color = clr::get(clr::PFL_RANBW,VERT_COLOR(&buffer,i) % CLR_RANBW_COUNT,clr::ALF_HALF);
				marker = (enum hud::markType)(VERT_COLOR(&buffer,i) / CLR_RANBW_COUNT);
				
				break;
			case VIEW_BONES:
				color = clr::get(clr::PFL_RANBW,VERT_BONE(&buffer,i) % CLR_RANBW_COUNT,clr::ALF_HALF);
				marker = (enum hud::markType)(VERT_BONE(&buffer,i) / CLR_RANBW_COUNT);
				
				break;
		}
		
		hud::drawMark(marker,drawX,drawY,false,POINT_RADIUS,color);
	}
}

// Selections -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::selectVert_Nearest(){
	if(!NEARVERT_VALID()){
		return;
	}
	
	// Toggle all with same coordinates as nearVert
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(VERT_X(&buffer,i) == VERT_X(&buffer,nearVert) && VERT_Y(&buffer,i) == VERT_Y(&buffer,nearVert)){
			selVerts[i] = !selVerts[i];
			
			if(selVerts[i]){
				++selVertCount;
			}else{
				--selVertCount;
			}
		}
	}
}

void vertLayer::selectVert_All(){
	memset(selVerts,1,buffer.count * TRI_VERT_COUNT * sizeof(unsigned char));
	selVertCount = buffer.count * TRI_VERT_COUNT;
}

void vertLayer::selectVert_Clear(){
	memset(selVerts,0,SEL_VERTS_COUNT() * sizeof(unsigned char));
	selVertCount = 0;
}

void vertLayer::selectVert_SelectedTris(){
	selVertCount = 0;
	
	for(unsigned int i = 0;i < buffer.count;++i){
		selVerts[3 * i + 0] = selTris[i];
		selVerts[3 * i + 1] = selTris[i];
		selVerts[3 * i + 2] = selTris[i];
		
		selVertCount += 3 * (selTris[i] > 0);
	}
}

void vertLayer::selectVert_ByBone(unsigned char bone){
	selVertCount = 0;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		selVerts[i] = (VERT_BONE(&buffer,i) == bone);
		
		if(selVerts[i]){
			++selVertCount;
		}
	}
}

void vertLayer::selectVert_ByColor(unsigned char color){
	selVertCount = 0;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		selVerts[i] = (VERT_COLOR(&buffer,i) == color);
		
		if(selVerts[i]){
			++selVertCount;
		}
	}
}

void vertLayer::selectTri_Nearest(){
	if(!NEARTRI_VALID()){
		return;
	}
	
	selTris[nearTri] = !selTris[nearTri];
	modified = true;
}

void vertLayer::selectTri_All(){
	memset(selTris,1,buffer.count * sizeof(unsigned char));
	modified = true;
}

void vertLayer::selectTri_Clear(){
	memset(selTris,0,SEL_TRIS_COUNT() * sizeof(unsigned char));
	modified = true;
}

void vertLayer::selectTri_SelectedVerts(){
	for(unsigned int i = 0;i < buffer.count;++i){
		selTris[i] = selVerts[TRI_V(i,0)] && selVerts[TRI_V(i,1)] && selVerts[TRI_V(i,2)];
	}
	
	modified = true;
}

// Buffer Operations -------------------------------------------------------------------------------------------------------------------------------------------
void vertLayer::nearVert_SetColor(unsigned char color){
	bool set = false;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(i / TRI_VERT_COUNT == nearTri && VERT_X(&buffer,i) == VERT_X(&buffer,nearVert) && VERT_Y(&buffer,i) == VERT_Y(&buffer,nearVert)){
			VERT_COLOR(&buffer,i) = color;
			set = true;
		}
	}
	
	modified = modified || set;
}

void vertLayer::nearVert_SetBone(unsigned char bone){
	bool set = false;
	
	for(unsigned int i = 0;i < buffer.count * TRI_VERT_COUNT;++i){
		if(i / TRI_VERT_COUNT == nearTri && VERT_X(&buffer,i) == VERT_X(&buffer,nearVert) && VERT_Y(&buffer,i) == VERT_Y(&buffer,nearVert)){
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
	for(unsigned int i = 0;i < TRI_VERT_COUNT;++i){
		VERT_U(&buffer,TRI_V(buffer.count,i)) = VERT_X(&buffer,TRI_V(buffer.count,i));
		VERT_V(&buffer,TRI_V(buffer.count,i)) = VERT_Y(&buffer,TRI_V(buffer.count,i));
		
		VERT_TYPE(&buffer,TRI_V(buffer.count,i)) = type;
		VERT_COLOR(&buffer,TRI_V(buffer.count,i)) = 0;
		VERT_BONE(&buffer,TRI_V(buffer.count,i)) = 0;
	}
	
	// Selection clearance
	selVerts[TRI_V(buffer.count,0)] = 0;
	selVerts[TRI_V(buffer.count,1)] = 0;
	selVerts[TRI_V(buffer.count,2)] = 0;
	
	selTris[buffer.count] = 0;
	
	// Update state
	++buffer.count;
	modified = true;
}

void vertLayer::tris_DeleteSelected(){
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