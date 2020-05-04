#pragma once

#include <core/mat.h>
#include <core/func.h>

namespace nonEuc
{
	class World
	{
	public:
		//描述整个空间（三维流形）的性质

		//三维流形曲纹坐标->四维空间的映射(不一定用到)
		Func3to4 coord;
		//度规矩阵
		Func3to33 metric;
		//Jacobi矩阵
		Func3to43 jacobi;
		//度量的Christoffel记号
		Func3to333 gamma;
	
		
	};
}