////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2013
//
//    This file is part of Scorched3D.
//
//    Scorched3D is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Scorched3D is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
////////////////////////////////////////////////////////////////////////////////

#include <uistate/UIStatePlayingLand.h>
#include <graph/LandscapeBlenderFull.h>
#include <scorched3dc/ScorchedUI.h>
#include <scorched3dc/OgreSystem.h>
#include <common/simplexnoise.h>
#include <common/DefinesAssert.h>
#include <common/Logger.h>
#include <client/ScorchedClient.h>
#include <client/ClientOptions.h>
#include <console/ConsoleRuleMethodIAdapter.h>
#include <landscapemap/LandscapeMaps.h>
#include <landscapedef/LandscapeDescriptions.h>
#include <BatchPage.h>
#include <ImpostorPage.h>
#include <GrassLoader.h>

static UIStatePlayingLand *currentLand_ = 0;

UIStatePlayingLand::UIStatePlayingLand(
	Ogre::SceneManager* sceneMgr, 
	Ogre::Camera* camera,
	Ogre::Light *sunLight,
	Ogre::Light *shadowLight,
	Hydrax::Hydrax *hydrax) : 
	sceneMgr_(sceneMgr), camera_(camera), 
	sunLight_(sunLight), shadowLight_(shadowLight),
	hydrax_(hydrax), landscapeGrid_(0), hmap_(0)
{
	currentLand_ = this;
	create();
}

UIStatePlayingLand::~UIStatePlayingLand()
{
}

void UIStatePlayingLand::create()
{
	hmap_ = &ScorchedClient::instance()->getLandscapeMaps().getGroundMaps().getHeightMap();
	Ogre::Root *ogreRoot = ScorchedUI::instance()->getOgreSystem().getOgreRoot();
	Ogre::RenderWindow *ogreRenderWindow = ScorchedUI::instance()->getOgreSystem().getOgreRenderWindow();

	int landscapeWidth = ScorchedClient::instance()->getLandscapeMaps().getGroundMaps().getLandscapeWidth();
	int landscapeHeight = ScorchedClient::instance()->getLandscapeMaps().getGroundMaps().getLandscapeHeight();
	int landscapeSquaresWidth = landscapeWidth / 128;
	int landscapeSquaresHeight = landscapeHeight / 128;

	// Create the terrain objects
	terrainGroup_ = new Ogre::TerrainGroup(sceneMgr_, Ogre::Terrain::ALIGN_X_Z, 129, OgreSystem::OGRE_WORLD_SIZE);
	// Move the terrain so its bottom left is at 0,0 (the terrain normal counts the top as 0)
	terrainGroup_->setOrigin(Ogre::Vector3(OgreSystem::OGRE_WORLD_SIZE / 2, 0, 
		(landscapeSquaresHeight * OgreSystem::OGRE_WORLD_SIZE) - (OgreSystem::OGRE_WORLD_SIZE / 2)));

	// Load terrain data
	defineTerrainCreationOptions();
	for (int x = 0; x<landscapeSquaresWidth; x++)
	{
		for (int y=0; y<landscapeSquaresHeight; y++)
		{
			defineTerrain(x, y);
		}
	}

	// Load the terrain from the loaded data
	terrainGroup_->loadAllTerrains(true);

	// Generate the information required by all layers
	// including the normal map
	LayersInfo layersInfo;
	createLayerInfo(layersInfo, landscapeSquaresWidth, landscapeSquaresHeight);

	// Create the layers including blend maps
	size_t blendMapWidth, blendMapHeight;
	getBlendMapWidth(blendMapWidth, blendMapHeight);
	LandscapeBlenderFull landscapeBlender(
		*ScorchedClient::instance(), layersInfo,
		blendMapWidth, blendMapHeight,
		terrainGroup_);
	landscapeBlender.calculate(landscapeSquaresWidth, landscapeSquaresHeight);

	// Calculate the terrain
	terrainGroup_->update();
	terrainGroup_->freeTemporaryResources();

	// Create the grass and trees for the appropriate layers
	createGrass(layersInfo, landscapeSquaresWidth, landscapeSquaresHeight);
	createTrees(landscapeSquaresWidth, landscapeSquaresHeight);

	new ConsoleRuleMethodIAdapter<UIStatePlayingLand>(
		ScorchedClient::instance()->getConsole(),
		this, &UIStatePlayingLand::updateAllTerrainHeight,
		"LandscapeRecalculate", 
		"Forces the landscape to recalculate itself based on the information currently in the height map");
}

void UIStatePlayingLand::defineTerrainCreationOptions()
{
	LandscapeTex *tex = ScorchedClient::instance()->getLandscapeMaps().getDescriptions().getTex();
	DIALOG_ASSERT(tex->texture.getValue()->getType() == LandscapeTexBase::eTexLayers);
	LandscapeTexLayers *texLayers = (LandscapeTexLayers*) tex->texture.getValue();

	// Terrain settings
	// Configure global
	terrainGlobalOptions_ = new Ogre::TerrainGlobalOptions();
    terrainGlobalOptions_->setMaxPixelError(8);
    terrainGlobalOptions_->setCompositeMapDistance(10000); 
	terrainGlobalOptions_->setUseVertexCompressionWhenAvailable(false);
	terrainGlobalOptions_->setVisibilityFlags(OgreSystem::VisibiltyMaskLandscape);
	terrainGlobalOptions_->setDefaultResourceGroup(LANDSCAPE_RESOURCE_GROUP);

    // Important to set these so that the terrain knows what to use for derived (non-realtime) data
    terrainGlobalOptions_->setLightMapDirection(shadowLight_->getDerivedDirection());
    terrainGlobalOptions_->setCompositeMapAmbient(sceneMgr_->getAmbientLight());
    terrainGlobalOptions_->setCompositeMapDiffuse(shadowLight_->getDiffuseColour());

    // Configure default import settings for if we use imported image
    Ogre::Terrain::ImportData& defaultimp = terrainGroup_->getDefaultImportSettings();
    defaultimp.terrainSize = 129; 
    defaultimp.worldSize = OgreSystem::OGRE_WORLD_SIZE;   
	defaultimp.inputScale = OgreSystem::OGRE_WORLD_HEIGHT_SCALE; 
    defaultimp.minBatchSize = 33;
    defaultimp.maxBatchSize = 65;

    // textures
	defaultimp.layerList.resize(5);
	defaultimp.layerList[0].worldSize = (Ogre::Real) texLayers->texturelayerdetail.worldsize.getValue().asFloat();
	defaultimp.layerList[0].textureNames.push_back(texLayers->texturelayerdetail.texturename.getValue());
	defaultimp.layerList[0].textureNames.push_back(texLayers->texturelayerdetail.normaltexturename.getValue());
	defaultimp.layerList[1].worldSize = (Ogre::Real) texLayers->texturelayer1.worldsize.getValue().asFloat();
	defaultimp.layerList[1].textureNames.push_back(texLayers->texturelayer1.texturename.getValue());
	defaultimp.layerList[1].textureNames.push_back(texLayers->texturelayer1.normaltexturename.getValue());
	defaultimp.layerList[2].worldSize = (Ogre::Real) texLayers->texturelayer2.worldsize.getValue().asFloat();
	defaultimp.layerList[2].textureNames.push_back(texLayers->texturelayer2.texturename.getValue());
	defaultimp.layerList[2].textureNames.push_back(texLayers->texturelayer2.normaltexturename.getValue());
	defaultimp.layerList[3].worldSize = (Ogre::Real) texLayers->texturelayer3.worldsize.getValue().asFloat();
	defaultimp.layerList[3].textureNames.push_back(texLayers->texturelayer3.texturename.getValue());
	defaultimp.layerList[3].textureNames.push_back(texLayers->texturelayer3.normaltexturename.getValue());
	defaultimp.layerList[4].worldSize = (Ogre::Real) texLayers->texturelayerslope.worldsize.getValue().asFloat();
	defaultimp.layerList[4].textureNames.push_back(texLayers->texturelayerslope.texturename.getValue());
	defaultimp.layerList[4].textureNames.push_back(texLayers->texturelayerslope.normaltexturename.getValue());
}

void UIStatePlayingLand::getBlendMapWidth(size_t &blendMapWidth, size_t &blendMapHeight)
{
	// Do this rather than firstTerrain->getLayerBlendMapSize() as it seems to be wrong when compiled in release mode
	Ogre::Terrain* firstTerrain = terrainGroup_->getTerrain(0, 0);
	Ogre::TerrainLayerBlendMap* blendMap0 = firstTerrain->getLayerBlendMap(1);
	blendMap0->convertUVToImageSpace(1.0, 1.0, &blendMapWidth, &blendMapHeight);
	blendMapWidth++; // They are indexed by Zero
	blendMapHeight++;
}

void UIStatePlayingLand::createLayerInfo(LayersInfo &layersInfo, int landscapeSquaresWidth, int landscapeSquaresHeight)
{
	LandscapeTex *tex = ScorchedClient::instance()->getLandscapeMaps().getDescriptions().getTex();
	DIALOG_ASSERT(tex->texture.getValue()->getType() == LandscapeTexBase::eTexLayers);
	LandscapeTexLayers *texLayers = (LandscapeTexLayers*)tex->texture.getValue();

	// Find the most efficient size to create the maps
	// i.e. the same size as all the blend maps applied to the landscape
	size_t blendMapWidth = 0, blendMapHeight = 0;
	getBlendMapWidth(blendMapWidth, blendMapHeight);
	int fullImageWidth = landscapeSquaresWidth * blendMapWidth;
	int fullImageHeight = landscapeSquaresHeight * blendMapHeight;

	// Add the entire landscape normal data to an image
	// The image we can resize to match the blend map size
	// This image is smoothed during the resize process and is a quick/dirty way
	// of smoothing the landscape normal data over the blend map
	// We don't need to do this with the height data as we can get this from the landscape itself
	// Note: The memory for this is deleted by Ogre because the data is loaded into the image with autodeleete set
	unsigned char *rawData = OGRE_ALLOC_T(unsigned char, 
		(hmap_->getMapHeight() + 1)*(hmap_->getMapWidth() + 1)*sizeof(unsigned char), 
		Ogre::MEMCATEGORY_GENERAL);
	unsigned char *currentRawData = rawData;
	for (int y=0; y<hmap_->getMapHeight() + 1; y++)
	{
		HeightMap::HeightData *data = hmap_->getHeightData(y);
		for (int x=0; x<hmap_->getMapWidth() + 1; x++, currentRawData++, data++)
		{
			FixedVector &normal = data->normal;
			*currentRawData = (unsigned char) ((normal[2].asFloat() + 1.0f) * 126.0f);
		}
	}
	layersInfo.normalMapImage.loadDynamicImage(rawData, hmap_->getMapWidth() + 1, hmap_->getMapHeight() + 1, 1, Ogre::PF_L8, true);
	layersInfo.normalMapImage.resize(fullImageWidth, fullImageHeight);

	// Create the texture map image (contains the top down image of the landscape)
	rawData = OGRE_ALLOC_T(unsigned char,
		fullImageWidth * fullImageHeight * sizeof(unsigned char)* 3,
		Ogre::MEMCATEGORY_GENERAL);
	layersInfo.textureMapImage.loadDynamicImage(rawData, fullImageWidth, fullImageHeight,
		1, Ogre::PF_BYTE_RGB, true);

	// Initialize data thats per layer
	initLayer(layersInfo, texLayers->texturelayer1, fullImageWidth, fullImageHeight);
	initLayer(layersInfo, texLayers->texturelayer2, fullImageWidth, fullImageHeight);
	initLayer(layersInfo, texLayers->texturelayer3, fullImageWidth, fullImageHeight);
}

void UIStatePlayingLand::initLayer(LayersInfo &layersInfo, LandscapeTexLayer &layer,
	int fullImageWidth, int fullImageHeight)
{
	// Add the new layer
	LayerInfo *layerInfo = new LayerInfo();
	layersInfo.layers.push_back(layerInfo);
	layerInfo->texLayer = &layer;

	// Check if we need to create the grasss
	if (!layer.grassList.getChildren().empty())
	{
		layerInfo->hasGrass = true;

		// Create the grassmap density
		unsigned char *rawData = OGRE_ALLOC_T(unsigned char,
			fullImageWidth * fullImageHeight * sizeof(unsigned char),
			Ogre::MEMCATEGORY_GENERAL);
		layerInfo->grassLayerDensity.loadDynamicImage(rawData, fullImageWidth, fullImageHeight, 
			1, Ogre::PF_L8, true);
	}
}

void UIStatePlayingLand::defineTerrain(long tx, long ty)
{
	int startX = tx * 128;
	int startY = ty * 128;

	float *heightData = OGRE_ALLOC_T(float, 129*129, Ogre::MEMCATEGORY_GEOMETRY);
	float *currentPoint = heightData;
	for (int y=0; y<129; y++)
	{
		HeightMap::HeightData *data = hmap_->getHeightData(startX, hmap_->getMapHeight() - (startY + y));
		for (int x=0; x<129; x++, currentPoint++, data++)
		{
			float height = data->height.asFloat();
			*currentPoint = height;
		}
	}
	
	Ogre::Terrain::ImportData newImport(terrainGroup_->getDefaultImportSettings());
	newImport.inputFloat = heightData;
	newImport.deleteInputData = false;

	terrainGroup_->defineTerrain(tx, ty, &newImport);
}

void UIStatePlayingLand::updateLandscapeTextures()
{
	int landscapeWidth = ScorchedClient::instance()->getLandscapeMaps().getGroundMaps().getLandscapeWidth();
	int landscapeHeight = ScorchedClient::instance()->getLandscapeMaps().getGroundMaps().getLandscapeHeight();
	for (int x=0; x<landscapeWidth/128; x++)
	{
		for (int y=0; y<landscapeHeight/128; y++)
		{
			Ogre::Terrain* terrain = terrainGroup_->getTerrain(x, y);
			Ogre::MaterialPtr material = terrain->getMaterial();
			Ogre::Technique *technique = material->createTechnique();

			hydrax_->getMaterialManager()->addDepthTechnique(technique);
		}
	}
}

void UIStatePlayingLand::update(float frameTime)
{
	bool landscapeEnabled = (sceneMgr_->getVisibilityMask() & OgreSystem::VisibiltyMaskLandscape) != 0;
	if (landscapeEnabled != ClientOptions::instance()->getLandscapeDraw())
	{
		sceneMgr_->setVisibilityMask(sceneMgr_->getVisibilityMask() ^ OgreSystem::VisibiltyMaskLandscape);
	}
	if ((landscapeGrid_!=0) != ClientOptions::instance()->getLandscapeGridDraw()) 
	{
		if (ClientOptions::instance()->getLandscapeGridDraw()) showLandscapePoints();
		else hideLandscapePoints();
	}

	if (!dirtyTerrains_.empty())
	{
		std::set<Ogre::Terrain *>::iterator itor;
		for (itor = dirtyTerrains_.begin();
			itor != dirtyTerrains_.end();
			++itor)
		{
			(*itor)->update(); // TODO: Optimize to only do once
		}
		dirtyTerrains_.clear();
	}

	std::list<Forests::PagedGeometry *>::iterator pagedGeom = pagedGeom_.begin(),
		pagedGeomEnd = pagedGeom_.end();
	for (; pagedGeom != pagedGeomEnd; ++pagedGeom)
	{
		(*pagedGeom)->update();
	}
}

void UIStatePlayingLand::heightChanged(UIStatePlaying::DeformType type, const FixedVector &position, int width)
{
	updateHeightFromHeightMap(
		position[0].asInt() - width, position[1].asInt() - width,
		width * 2, width * 2);

	switch (type)
	{
	case UIStatePlaying::DeformTypeCircle:
		{
			Ogre::Vector3 ogrePosition(position[0].asFloat(), position[2].asFloat(), position[1].asFloat());
			std::list<Forests::TreeLoader2D *>::iterator itor = treeLoaders_.begin(),
				end = treeLoaders_.end();
			for (; itor != end; ++itor)
			{
				Forests::TreeLoader2D *treeLoader = *itor;
				treeLoader->deleteTrees(
					ogrePosition, (Ogre::Real) width);
			}
			std::list<Forests::PagedGeometry *>::iterator pagedGeom = pagedGeom_.begin(),
				pagedGeomEnd = pagedGeom_.end();
			for (; pagedGeom != pagedGeomEnd; ++pagedGeom)
			{
				Forests::PagedGeometry *geom = *pagedGeom;
				geom->reloadGeometryPages(ogrePosition, (Ogre::Real) width);
			}
		}
		break;
	case UIStatePlaying::DeformTypeSquare:
		{
			Ogre::Real x = position[0].asFloat();
			Ogre::Real y = position[1].asFloat();
			Ogre::Real size = width * 2.0f;
			Forests::TBounds area(x - size, y + size, x + size, y - size);
			std::list<Forests::TreeLoader2D *>::iterator itor = treeLoaders_.begin(),
				end = treeLoaders_.end();
			for (; itor != end; ++itor)
			{
				Forests::TreeLoader2D *treeLoader = *itor;
				treeLoader->deleteTrees(area);
			}
			std::list<Forests::PagedGeometry *>::iterator pagedGeom = pagedGeom_.begin(),
				pagedGeomEnd = pagedGeom_.end();
			for (; pagedGeom != pagedGeomEnd; ++pagedGeom)
			{
				Forests::PagedGeometry *geom = *pagedGeom;
				geom->reloadGeometryPages(area);
			}
		}
		break;
	}
}

void UIStatePlayingLand::updateHeightFromHeightMap(int x, int y, int w, int h)
{
	// Make the update slightly larger so the boundries between terrains are accounted for
	x -= 1; y -= 1; w += 2; h += 2;

	// Terrain height map counts 0 as the top
	// So change height map coords to terrain coords
	int landscapeHeight = hmap_->getMapHeight();
	y = landscapeHeight - (y + h);

	int left = x / 128;
	int top = y / 128;
	int right = (x + w) / 128;
	int bottom = (y + h) / 128;

	Ogre::Rect updateRect(x, y, x + w, y + h);
	updateHeightTerrain(left, top, updateRect);
	if (left != right && top != bottom)
	{
		updateHeightTerrain(right, top, updateRect);
		updateHeightTerrain(left, bottom, updateRect);
		updateHeightTerrain(right, bottom, updateRect);
	}
	else if (left != right)
	{
		updateHeightTerrain(right, top, updateRect);
	}
	else if (top != bottom)
	{
		updateHeightTerrain(left, bottom, updateRect);
	}
}

void UIStatePlayingLand::updateHeightTerrain(int tx, int ty, const Ogre::Rect &updateRect)
{
	int landscapeWidth = hmap_->getMapWidth();
	int landscapeHeight = hmap_->getMapHeight();
	int startX = tx * 128;
	int startY = ty * 128;

	if (startX < 0 || startY < 0) return;
	if (startX >= landscapeWidth || startY >= landscapeHeight) return;

	Ogre::Terrain* terrain = terrainGroup_->getTerrain(tx, ty);
	Ogre::Rect terrainRect(0, 0, 128, 128);
	Ogre::Rect changeRect(updateRect.left - startX, updateRect.top - startY, updateRect.right - startX, updateRect.bottom - startY);
	Ogre::Rect actualUpdate = terrainRect.intersect(changeRect);

	for (int y = actualUpdate.top; y <= actualUpdate.bottom; y++)
	{
		HeightMap::HeightData *data = hmap_->getHeightData(startX + actualUpdate.left, landscapeHeight - (startY + y));
		float *landscapeHeightData = terrain->getHeightData(actualUpdate.left, y);
		for (int x = actualUpdate.left; x <= actualUpdate.right; x++, landscapeHeightData++, data++)
		{
			float height = data->height.getInternalData() * OgreSystem::OGRE_WORLD_HEIGHT_SCALE_FIXED;
			*landscapeHeightData = height;
		}
	}

	terrain->dirtyRect(Ogre::Rect(actualUpdate.left, actualUpdate.top,
		actualUpdate.right + 1, actualUpdate.bottom + 1));
	dirtyTerrains_.insert(terrain);
}

void UIStatePlayingLand::updateAllTerrainHeight()
{
	int landscapeWidth = hmap_->getMapWidth();
	int landscapeHeight = hmap_->getMapHeight();

	for (int tx = 0; tx<landscapeWidth / 128; tx++)
	{
		for (int ty = 0; ty<landscapeHeight / 128; ty++)
		{
			Ogre::Terrain* terrain = terrainGroup_->getTerrain(tx, ty);
			int startX = tx * 128;
			int startY = ty * 128;

			for (int y = 0; y<129; y++)
			{
				HeightMap::HeightData *data = hmap_->getHeightData(startX, landscapeHeight - (startY + y));
				float *landscapeHeightData = terrain->getHeightData(0, y);
				for (int x = 0; x<129; x++, landscapeHeightData++, data++)
				{
					float height = data->height.getInternalData() * OgreSystem::OGRE_WORLD_HEIGHT_SCALE_FIXED;
					*landscapeHeightData = height;
				}
			}

			terrain->dirtyRect(Ogre::Rect(0, 0, 129, 129));
			dirtyTerrains_.insert(terrain);
		}
	}
}

void UIStatePlayingLand::showLandscapePoints() 
{
	// A grid of lines showing the landscape as it is in the height map
	// This could fail in a horrible way as the hmap is being accessed while the client thread is running
	// Oh, well cba synchronizing for this debug case
	Ogre::SceneManager *sceneManager = ScorchedUI::instance()->getOgreSystem().getOgreLandscapeSceneManager();
	landscapeGrid_ = sceneManager->getRootSceneNode()->createChildSceneNode("LandscapeGrid");
	HeightMap::HeightData *heightData = hmap_->getHeightData();
	for (int y=0; y<hmap_->getMapHeight() + 1; y++)
	{
		Ogre::ManualObject* manual = sceneManager->createManualObject();
		manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);
		for (int x=0; x<hmap_->getMapWidth() + 1; x++, heightData++)
		{
			manual->position(OgreSystem::OGRE_WORLD_SCALE * x, 
				heightData->height.asFloat() * OgreSystem::OGRE_WORLD_HEIGHT_SCALE + 0.1f, 
				OgreSystem::OGRE_WORLD_SCALE * y);
		}
		manual->end();
		landscapeGrid_->attachObject(manual);
	}
}

void UIStatePlayingLand::hideLandscapePoints()
{
	OgreSystem::destroySceneNode(landscapeGrid_);
	landscapeGrid_ = 0;
}

// Used by paging scene manager
float UIStatePlayingLand::getTerrainHeight(const float x, const float z, void *userData)
{
	float height = currentLand_->terrainGroup_->getHeightAtWorldPosition(x, 0, z);
	return height;
}

Ogre::Real UIStatePlayingLand::getHeight(const Ogre::Vector3 &position)
{
	float height = currentLand_->terrainGroup_->getHeightAtWorldPosition(position.x, 0, position.z);
	return height;
}

bool UIStatePlayingLand::getIntersection(const Ogre::Ray &cameraRay, Ogre::Vector3 *outPosition)
{
	Ogre::TerrainGroup::RayResult result =
		terrainGroup_->rayIntersects(cameraRay);
	if (result.hit)
	{
		*outPosition = result.position;
		return true;
	}
	return false;
}

void UIStatePlayingLand::createGrass(LayersInfo &layersInfo, int landscapeSquaresWidth, int landscapeSquaresHeight)
{
	int g = 0;
	std::vector<LayerInfo *>::iterator itor = layersInfo.layers.begin(),
		end = layersInfo.layers.end();
	for (; itor != end; ++itor)
	{
		LayerInfo *layerInfo = *itor;
		std::list<LandscapeGrass *>::iterator gitor = layerInfo->texLayer->grassList.getChildren().begin(),
			gend = layerInfo->texLayer->grassList.getChildren().end();
		for (; gitor != gend; ++gitor)
		{
			LandscapeGrass *landscapeGrass = *gitor;

			// Paged Geometry
			Forests::PagedGeometry *grass = new Forests::PagedGeometry(camera_, 30);
			pagedGeom_.push_back(grass);
			grass->setTempDir(S3D::getTempFile("./"));
			grass->addDetailLevel<Forests::GrassPage>(OgreSystem::OGRE_WORLD_SCALE * 
				landscapeGrass->visibleDistance.getValue().asFloat());
			Forests::GrassLoader *grassLoader = new Forests::GrassLoader(grass);
			grass->setPageLoader(grassLoader);
			grassLoader->setHeightFunction(&getTerrainHeight);
			Forests::GrassLayer *layer = grassLoader->addLayer(landscapeGrass->materialName.getValue());
			float minimumSize = landscapeGrass->minimumSize.getValue().asFloat();
			layer->setMinimumSize(minimumSize, minimumSize);
			float maximumSize = landscapeGrass->maximumSize.getValue().asFloat();
			layer->setMaximumSize(maximumSize, maximumSize);
			layer->setAnimationEnabled(false);		//Enable animations
			layer->setSwayDistribution(7.0f);		//Sway fairly unsynchronized
			layer->setSwayLength(0.5f);				//Sway back and forth 0.5 units in length
			layer->setSwaySpeed(0.4f);				//Sway 1/2 a cycle every second
			layer->setDensity(landscapeGrass->density.getValue().asFloat());
			//layer->setRenderTechnique(Forests::GRASSTECH_SPRITE); // Sprits don't look good from above
			layer->setFadeTechnique(Forests::FADETECH_GROW);
			
			Ogre::TextureManager &textureManager = Ogre::TextureManager::getSingleton();
			Ogre::TexturePtr densityTexture = textureManager.loadImage(
				"PSMDensityMap", "General", layerInfo->grassLayerDensity, Ogre::TEX_TYPE_2D, 1);
			layer->setDensityMap(densityTexture);
			textureManager.remove("PSMDensityMap"); // Clean up texture, texture data is copied by PSM

			Ogre::TexturePtr colorTexture = textureManager.loadImage(
				"PSMColorMap", "General", layersInfo.textureMapImage, Ogre::TEX_TYPE_2D, 1);
			layer->setColorMap(colorTexture);
			textureManager.remove("PSMColorMap"); // Clean up texture, texture data is copied by PSM

			layer->setMapBounds(Forests::TBounds(0, 0,
				OgreSystem::OGRE_WORLD_SIZE * landscapeSquaresWidth,
				OgreSystem::OGRE_WORLD_SIZE * landscapeSquaresHeight));
		}
	}
}

void UIStatePlayingLand::createTrees(int landscapeSquaresWidth, int landscapeSquaresHeight)
{
	LandscapeTex *tex = ScorchedClient::instance()->getLandscapeMaps().getDescriptions().getTex();
	std::list<LandscapeTrees *>::iterator itor = tex->trees.getChildren().begin(),
		end = tex->trees.getChildren().end();
	for (; itor != end; ++itor)
	{
		// Get the placements for the trees
		LandscapeTrees *treesDefinition = *itor;
		FileRandomGenerator generator;
		std::list<FixedVector> positions;
		treesDefinition->getPositions(*hmap_, generator, positions, 0);
		
		if (!positions.empty())
		{
			// Create the trees instance
			Forests::PagedGeometry *trees = new Forests::PagedGeometry();
			trees->setTempDir(S3D::getTempFile("./"));
			trees->setCamera(camera_);	//Set the camera so PagedGeometry knows how to calculate LODs
			trees->setPageSize(200);	//Set the size of each page of geometry
			trees->setInfinite();		//Use infinite paging mode
			trees->addDetailLevel<Forests::BatchPage>(2000, 30);		// Use batches up to 150 units away, and fade for 30 more units
			trees->addDetailLevel<Forests::ImpostorPage>(OgreSystem::OGRE_WORLD_SIZE, 50);	// Use impostors up to 400 units, and for for 50 more units
			pagedGeom_.push_back(trees);

			// Create a new TreeLoader2D object
			Forests::TreeLoader2D *treeLoader = new Forests::TreeLoader2D(trees, Forests::TBounds(0, 0,
				OgreSystem::OGRE_WORLD_SIZE * landscapeSquaresWidth,
				OgreSystem::OGRE_WORLD_SIZE * landscapeSquaresHeight));
			trees->setPageLoader(treeLoader);	// Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance
			treeLoader->setHeightFunction(&getTerrainHeight); //Supply a height function to TreeLoader2D so it can calculate tree Y values
			treeLoaders_.push_back(treeLoader);
			
			// Add all of the tree objects
			Logger::log(S3D::formatStringBuffer("Adding %u trees", positions.size()));
			static unsigned int treeId = 0;
			std::string entityName = S3D::formatStringBuffer("Forests::PagedGeometry::Tree_%u", ++treeId);
			Ogre::Entity *tree1 = sceneMgr_->createEntity(entityName, treesDefinition->getMeshName());
			Ogre::Real globalScale = (Ogre::Real) treesDefinition->getMeshScale().asFloat();
			Ogre::Vector3 oposition;
			std::list<FixedVector>::iterator titor = positions.begin(), tend = positions.end();
			for (; titor != tend; ++titor)
			{
				Ogre::Real scale = Ogre::Math::RangeRandom(0.25f, 1.0f) * globalScale;
				Ogre::Radian yaw = Ogre::Degree(Ogre::Math::RangeRandom(0, 360));
				oposition.x = (*titor)[0].getInternalData() * OgreSystem::OGRE_WORLD_SCALE_FIXED;
				oposition.z = (*titor)[1].getInternalData() * OgreSystem::OGRE_WORLD_SCALE_FIXED;

				treeLoader->addTree(tree1, oposition, yaw, scale);
			}
		}
	}
}