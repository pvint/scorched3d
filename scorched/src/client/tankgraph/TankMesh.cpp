////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2009
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
//    You should have received a copy of the GNU General Public License
//    along with Scorched3D; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <tankgraph/TankMesh.h>
#include <GLEXT/GLState.h>
#include <graph/ModelRenderer.h>
#include <graph/OptionsDisplay.h>
#include <graph/ModelRenderer.h>
#include <graph/ModelRendererStore.h>
#include <graph/ModelRendererSimulator.h>
#include <common/Defines.h>

TankMesh::TankMesh(Model &tank) : 
	ModelRendererMesh(&tank),
	scale_(1.0f)
{
	setupTankMesh();
}

TankMesh::~TankMesh()
{
}

int TankMesh::getNoTris()
{
	return (model_?model_->getNumberTriangles():0);
}

void TankMesh::setupTankMesh()
{
	// Make sure the tank is not too large
	const float maxSize = 3.0f;
	Vector min = model_->getMin().asVector();
	Vector max = model_->getMax().asVector();
	float size = (max - min).Magnitude();
	if (size > maxSize) scale_ = 2.2f / size;

	// Find the turrets and guns
	Mesh *turretPivot = 0, *gunPivot = 0;
	int turretCount = 0;

	std::vector<Mesh *>::iterator itor;
	for (itor = model_->getMeshes().begin();
		itor != model_->getMeshes().end();
		itor++)
	{
		Mesh *mesh = (*itor);
		const char *name = mesh->getName();

		if (strstr(name, "\"Turret") == name ||
			strstr(name, "\"turret") == name)
		{
			if (strstr(name, "pivot") ||
				strstr(name, "Pivot"))
			{
				turretPivot = mesh;
			}
			else
			{
				// Find the center that the tank should rotate around
				turretCount++;
				turretCenter_ += (mesh->getMax() + mesh->getMin()) / 2;
			}

			meshTypes_.push_back(eTurret);
		}
		else if (strstr(name, "\"Gun") == name ||
			strstr(name, "\"gun") == name)
		{
			if (strstr(name, "pivot") ||
				strstr(name, "Pivot"))
			{
				gunPivot = mesh;
			}

			meshTypes_.push_back(eGun);
		}
		else
		{
			meshTypes_.push_back(eNone);
		}
	}

	// Find the center of rotation for the turret
	if (turretPivot)
	{
		turretCenter_ = (turretPivot->getMax() + turretPivot->getMin()) / 2;
	}
	else
	{
		turretCenter_ /= turretCount;
	}
	FixedVector gunCenter = turretCenter_;
	turretCenter_[2] = 0;

	// Find the center of rotation for the gun
	if (gunPivot)
	{
		gunCenter = (gunPivot->getMax() + gunPivot->getMin()) / 2;
	}
	gunOffset_ = gunCenter - turretCenter_;
}

void TankMesh::draw(float frame, float *rotMatrix, Vector &position, 
					float fireOffset, float rotXY, float rotXZ,
					bool absCenter, float scale, float fade, bool setState)
{
	rotXY_ = rotXY;
	rotXZ_ = rotXZ;
	fireOffSet_ = fireOffset;

	glPushMatrix();
		glTranslatef(position[0], position[1], position[2]);
		glMultMatrixf(rotMatrix);
		glScalef(scale * scale_, scale * scale_, scale * scale_);

		if (absCenter) ModelRendererMesh::draw(frame, 0.0f, fade, setState);
		else ModelRendererMesh::drawBottomAligned(frame, 0.0f, fade, setState);
	glPopMatrix();
}

void TankMesh::drawMesh(unsigned int m, Mesh *mesh, float currentFrame, bool setState)
{
	glPushMatrix();
		MeshType type = meshTypes_[m];
		vertexTranslation_ = -turretCenter_;

		if (type == eTurret || type == eGun)
		{
			glRotatef(rotXY_, 0.0f, 0.0f, 1.0f);
			if (type == eGun)
			{
				glTranslatef(gunOffset_[0].asFloat(), gunOffset_[1].asFloat(), gunOffset_[2].asFloat());
				vertexTranslation_ -= gunOffset_;
				glRotatef(rotXZ_, 1.0f, 0.0f, 0.0f);

				if (fireOffSet_ != 0.0f) glTranslatef(0.0f, fireOffSet_, 0.0f);
			}
		}

		ModelRendererMesh::drawMesh(m, mesh, currentFrame, setState);
	glPopMatrix();

	vertexTranslation_.zero();
}
