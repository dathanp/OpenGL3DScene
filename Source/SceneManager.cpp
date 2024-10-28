///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 * DefineObjectMaterials()
 *
 * This method is used for configuring the various material
 * settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help ***/
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	plasticMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.shininess = 5.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.5f, 0.2f, 0.5f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	stoneMaterial.specularColor = glm::vec3(0.73f, 0.3f, 0.3f);
	stoneMaterial.shininess = 6.0;
	stoneMaterial.tag = "stone";
	m_objectMaterials.push_back(stoneMaterial);

}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 4.0f, 6.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", -4.0f, -4.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// directional light 3
	m_pShaderManager->setVec3Value("directionalLight.direction", 7.2f, 7.2f, 1.5f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.05f, 0.05f, 0.01f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	
	// spotlight 4
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.014f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.0007f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(22.5f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff",
		glm::cos(glm::radians(28.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);


}

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	CreateGLTexture("textures/marbletexture.jpg", "marble");
	CreateGLTexture("textures/woodtexture.jpg", "wood");



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::vec3 scaleCylinderBody;
	glm::vec3 scaleTaperedCylinder;
	glm::vec3 positionCylinderBody;
	glm::vec3 positionTaperedCylinder;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(40.0f, 0.5f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.8f, 0.6f, 0.4f, 1.0f);
	SetShaderMaterial("wood");
	SetShaderTexture("marble");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	// Table Legs 
	float tableHeight = 3.0f; 
	glm::vec3 scaleLeg = glm::vec3(0.3f, tableHeight, 0.3f);  

	// Tabletop dimensions
	float tableWidth = 40.0f;   
	float tableDepth = 20.0f;   
	float legOffsetX = scaleLeg.x * 2.0f; 
	float legOffsetZ = scaleLeg.z * 1.5f; 

	// Table leg positions
	glm::vec3 legPositions[4] = {
		glm::vec3(-tableWidth / 2.0f + legOffsetX, -tableHeight / 1.0f, -tableDepth / 2.0f + legOffsetZ), // Rear-left corner
		glm::vec3(tableWidth / 2.0f - legOffsetX, -tableHeight / 1.0f, -tableDepth / 2.0f + legOffsetZ),  // Rear-right corner
		glm::vec3(-tableWidth / 2.0f + legOffsetX, -tableHeight / 1.0f, tableDepth / 2.0f - legOffsetZ),  // Front-left corner
		glm::vec3(tableWidth / 2.0f - legOffsetX, -tableHeight / 1.0f, tableDepth / 2.0f - legOffsetZ)    // Front-right corner
	};
	
	// Render the four legs
	for (int i = 0; i < 4; i++) {
		SetTransformations(scaleLeg, 0.0f, 0.0f, 0.0f, legPositions[i]);
		//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
		SetShaderTexture("wood");
		m_basicMeshes->DrawCylinderMesh();
	}


	/****************************************************************/
	//Main Cylinder (body of the bowl)
	scaleCylinderBody = glm::vec3(1.5f, 0.9f, 1.5f); 
	positionCylinderBody = glm::vec3(0.0f, 0.0f, 0.0f);  

	SetTransformations(scaleCylinderBody, 0.0f, 0.0f, 0.0f, positionCylinderBody);
	SetShaderColor(1,1,1,1); 
	SetShaderMaterial("plastic");
	//SetShaderTexture("marble");
	m_basicMeshes->DrawCylinderMesh();

	//Tapered Cylinder (upper slope of the bowl)
	scaleTaperedCylinder = glm::vec3(2.0f, 0.3f, 2.0f);  
	positionTaperedCylinder = glm::vec3(0.0f, 1.0f, 0.0f);
	float rotationDegrees = 180.0f;

	SetTransformations(scaleTaperedCylinder, rotationDegrees, 0.0f, 0.0f, positionTaperedCylinder);
	//SetShaderColor(1,1,1,1);  
	SetShaderMaterial("stone");
	SetShaderTexture("marble");
	m_basicMeshes->DrawTaperedCylinderMesh();

	//Microwave
	// Set transformations for microwave body 
	glm::vec3 scaleMicrowaveBody = glm::vec3(9.5f, 5.2f, 5.5f); 
	glm::vec3 positionMicrowaveBody = glm::vec3(10.0f, 3.0f, 0.0f);

	SetTransformations(scaleMicrowaveBody, 0.0f, 0.0f, 0.0f, positionMicrowaveBody);
	SetShaderMaterial("plastic");
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Microwave front panel
	glm::vec3 scaleMicrowaveFront = glm::vec3(9.5f, 5.2f, 0.1f); 
	glm::vec3 positionMicrowaveFront = glm::vec3(10.0f, 3.0f, 2.8f); 

	SetTransformations(scaleMicrowaveFront, 0.0f, 0.0f, 0.0f, positionMicrowaveFront);
	SetShaderMaterial("plastic");
	SetShaderTexture("marble"); 
	m_basicMeshes->DrawBoxMesh();

	// Microwave control panel 
	glm::vec3 scaleMicrowavePanel = glm::vec3(0.3f, 0.7f, 1.5f); 
	glm::vec3 positionMicrowavePanel = glm::vec3(13.2f, 1.3f, 2.85f); 

	SetTransformations(scaleMicrowavePanel, 0.0f, 90.0f, 0.0f, positionMicrowavePanel);
	SetShaderMaterial("plastic");
	SetShaderTexture("wood"); 
	m_basicMeshes->DrawBoxMesh(); 

	// Ice maker 
	glm::vec3 scaleIceMakerBody = glm::vec3(4.5f, 5.0f, 4.2f); 
	glm::vec3 positionIceMakerBody = glm::vec3(-5.0f, 2.7f, 0.0f); 

	SetTransformations(scaleIceMakerBody, 0.0f, 0.0f, 0.0f, positionIceMakerBody);
	SetShaderMaterial("plastic");
	SetShaderColor(0.8f, 0.1f, 0.1f, 1.0f); 
	m_basicMeshes->DrawBoxMesh();

	// Ice maker front 
	glm::vec3 scaleIceMakerFrontCylinder = glm::vec3(2.27f, 5.0f, 1.8f); 
	glm::vec3 positionIceMakerFrontCylinder = glm::vec3(-5.0f, 0.2f, 1.96f);

	SetTransformations(scaleIceMakerFrontCylinder, 0.0f, 0.0f, 0.0f, positionIceMakerFrontCylinder); 
	SetShaderMaterial("plastic");
	SetShaderColor(0.8f, 0.1f, 0.1f, 1.0f); 
	m_basicMeshes->DrawCylinderMesh(); 

	// Pitcher
	// Main body of the pitcher 
	glm::vec3 scalePitcherBody = glm::vec3(1.0f, 2.5f, 1.0f); 
	glm::vec3 positionPitcherBody = glm::vec3(1.5f, 0.0f, -4.0f);

	SetTransformations(scalePitcherBody, 0.0f, 0.0f, 0.0f, positionPitcherBody);
	SetShaderMaterial("plastic"); 
	SetShaderColor(0.4f, 0.9f, 0.9f, 4.0f); 
	m_basicMeshes->DrawCylinderMesh(); 

	// Pitcher spout 
	glm::vec3 scalePitcherSpout = glm::vec3(0.2f, 0.3f, 0.3f); 
	glm::vec3 positionPitcherSpout = glm::vec3(1.5f, 1.9f, -3.2f); 

	SetTransformations(scalePitcherSpout, 45.0f, 0.0f, 0.0f, positionPitcherSpout); 
	SetShaderMaterial("plastic");
	SetShaderColor(0.4f, 0.9f, 0.9f, 4.0f);
	m_basicMeshes->DrawCylinderMesh(); 
}
