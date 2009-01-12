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

#if !defined(AFX_DEFORMCIRCULAR_H__7E191509_3CD0_4EA5_A6B2_7C96C081C1AD__INCLUDED_)
#define AFX_DEFORMCIRCULAR_H__7E191509_3CD0_4EA5_A6B2_7C96C081C1AD__INCLUDED_

class ScorchedContext;
class ProgressCounter;

#include <common/FixedVector.h>
#include <vector>

class DeformLandscape
{
public:
	struct DeformPoints
	{
		fixed map[100][100];
	};
	enum DeformType
	{
		eDeformLandscapeUp = 1,
		eDeformLandscapeDown = 2,
		eFlattenArea = 3,
	};
	struct DeformInfo
	{
		int type;
		FixedVector pos;
		fixed radius;
	};

	static bool deformLandscape(
		ScorchedContext &context,
		FixedVector &pos, fixed radius, 
		bool down, DeformPoints &map);
	static void flattenArea(
		ScorchedContext &context, 
		FixedVector &tankPos,
		bool removeObjects = true,
		fixed size = 2);

	static void clearInfos();
	static std::vector<DeformInfo> &getInfos() { return deformInfos_; }
	static void applyInfos(ScorchedContext &context, 
		std::vector<DeformInfo> &infos,
		ProgressCounter *counter = 0);

private:
	static std::vector<DeformInfo> deformInfos_;

	static bool deformLandscapeInternal(
		ScorchedContext &context,
		FixedVector &pos, fixed radius, 
		bool down, DeformPoints &map,
		bool setNormals);
	static void flattenAreaInternal(
		ScorchedContext &context, 
		FixedVector &tankPos,
		bool removeObjects,
		fixed size,
		bool setNormals);
};

#endif // !defined(AFX_DEFORMCIRCULAR_H__7E191509_3CD0_4EA5_A6B2_7C96C081C1AD__INCLUDED_)