/** 
 * @file llvertexbuffer.h
 * @brief LLVertexBuffer wrapper for OpengGL vertex buffer objects
 *
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#ifndef LL_LLVERTEXBUFFER_H
#define LL_LLVERTEXBUFFER_H

#include "llgl.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "v4coloru.h"
#include "llstrider.h"
#include "llmemory.h"
#include <set>
#include <vector>

//============================================================================
// NOTES
// Threading:
//  All constructors should take an 'create' paramater which should only be
//  'true' when called from the main thread. Otherwise createGLBuffer() will
//  be called as soon as getVertexPointer(), etc is called (which MUST ONLY be
//  called from the main (i.e OpenGL) thread)

//============================================================================
// base class

class LLVertexBuffer : public LLRefCount
{
public:
	static void initClass(bool use_vbo);
	static void cleanupClass();
 	static void startRender(); //between start and stop render, no client copies will occur
	static void stopRender(); //any buffer not copied to GL will be rendered from client memory
	static void clientCopy(F64 max_time = 0.005); //copy data from client to GL
	static void unbind(); //unbind any bound vertex buffer

	enum {
		TYPE_VERTEX,
		TYPE_NORMAL,
		TYPE_TEXCOORD,
		TYPE_TEXCOORD2,
		TYPE_COLOR,
		// These use VertexAttribPointer and should possibly be made generic
		TYPE_BINORMAL,
		TYPE_WEIGHT,
		TYPE_CLOTHWEIGHT,
		TYPE_MAX,
		TYPE_INDEX,
	};
	enum {
		MAP_VERTEX = (1<<TYPE_VERTEX),
		MAP_NORMAL = (1<<TYPE_NORMAL),
		MAP_TEXCOORD = (1<<TYPE_TEXCOORD),
		MAP_TEXCOORD2 = (1<<TYPE_TEXCOORD2),
		MAP_COLOR = (1<<TYPE_COLOR),
		// These use VertexAttribPointer and should possibly be made generic
		MAP_BINORMAL = (1<<TYPE_BINORMAL),
		MAP_WEIGHT = (1<<TYPE_WEIGHT),
		MAP_CLOTHWEIGHT = (1<<TYPE_CLOTHWEIGHT),
		MAP_DRAW = 0x2000, // Buffer is in draw (read-only) mode
		MAP_MAPPED = 0x4000, // Indicates that buffer has been mapped, but not to any type of data
		MAP_UNMAPPED = 0x8000 // Indicates that buffer has been logically un-mapped
	};
	
protected:
	virtual ~LLVertexBuffer(); // use unref()

	virtual void setupVertexBuffer(U32 data_mask) const; // pure virtual, called from mapBuffer()

	void	createGLBuffer();
	void	createGLIndices();
	void 	destroyGLBuffer();
	void 	destroyGLIndices();
	void	updateNumVerts(S32 nverts);
	void	updateNumIndices(S32 nindices); 
	virtual BOOL	useVBOs() const;
	void	unmapBuffer();
	
public:
	LLVertexBuffer(U32 typemask, S32 usage);
	
	// map for data access
	U8*		mapBuffer(S32 access = -1);
	// set for rendering
	virtual void	setBuffer(U32 data_mask); 	// calls  setupVertexBuffer() if data_mask is not 0
	// allocate buffer
	void	allocateBuffer(S32 nverts, S32 nindices, bool create);
	virtual void resizeBuffer(S32 newnverts, S32 newnindices);
	void	makeStatic();
	
	// Only call each getVertexPointer, etc, once before calling unmapBuffer()
	// call unmapBuffer() after calls to getXXXStrider() before any cals to setBuffer()
	// example:
	//   vb->getVertexBuffer(verts);
	//   vb->getNormalStrider(norms);
	//   setVertsNorms(verts, norms);
	//   vb->unmapBuffer();
	bool getVertexStrider(LLStrider<LLVector3>& strider, S32 index=0);
	bool getIndexStrider(LLStrider<U32>& strider, S32 index=0);
	bool getTexCoordStrider(LLStrider<LLVector2>& strider, S32 index=0);
	bool getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index=0);
	bool getNormalStrider(LLStrider<LLVector3>& strider, S32 index=0);
	bool getBinormalStrider(LLStrider<LLVector3>& strider, S32 index=0);
	bool getColorStrider(LLStrider<LLColor4U>& strider, S32 index=0);
	bool getWeightStrider(LLStrider<F32>& strider, S32 index=0);
	bool getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index=0);
	
	BOOL isEmpty() const					{ return mEmpty; }
	BOOL isLocked() const					{ return mLocked; }
	S32 getNumVerts() const					{ return mNumVerts; }
	S32 getNumIndices() const				{ return mNumIndices; }
	U8* getIndicesPointer() const			{ return useVBOs() ? NULL : mMappedIndexData; }
	U8* getVerticesPointer() const			{ return useVBOs() ? NULL : mMappedData; }
	S32 getStride() const					{ return mStride; }
	S32 getTypeMask() const					{ return mTypeMask; }
	BOOL hasDataType(S32 type) const		{ return ((1 << type) & getTypeMask()) ? TRUE : FALSE; }
	S32 getSize() const						{ return mNumVerts*mStride; }
	S32 getIndicesSize() const				{ return mNumIndices * sizeof(U32); }
	U8* getMappedData() const				{ return mMappedData; }
	U8* getMappedIndices() const			{ return mMappedIndexData; }
	S32 getOffset(S32 type) const			{ return mOffsets[type]; }
	S32 getUsage() const					{ return mUsage; }

	void setStride(S32 type, S32 new_stride);
	
	void markDirty(U32 vert_index, U32 vert_count, U32 indices_index, U32 indices_count);
	void markClean();

protected:	
	S32		mNumVerts;		// Number of vertices
	S32		mNumIndices;	// Number of indices
	S32		mStride;
	U32		mTypeMask;
	S32		mUsage;			// GL usage
	U32		mGLBuffer;		// GL VBO handle
	U32		mGLIndices;		// GL IBO handle
	U8*		mMappedData;	// pointer to currently mapped data (NULL if unmapped)
	U8*		mMappedIndexData;	// pointer to currently mapped indices (NULL if unmapped)
	BOOL	mLocked;			// if TRUE, buffer is being or has been written to in client memory
	BOOL	mFinal;			// if TRUE, buffer can not be mapped again
	BOOL	mFilthy;		// if TRUE, entire buffer must be copied (used to prevent redundant dirty flags)
	BOOL	mEmpty;			// if TRUE, client buffer is empty (or NULL). Old values have been discarded.
	S32		mOffsets[TYPE_MAX];
	BOOL	mResized;		// if TRUE, client buffer has been resized and GL buffer has not
	BOOL	mDynamicSize;	// if TRUE, buffer has been resized at least once (and should be padded)

	class DirtyRegion
	{
	public:
		U32 mIndex;
		U32 mCount;
		U32 mIndicesIndex;
		U32 mIndicesCount;

		DirtyRegion(U32 vi, U32 vc, U32 ii, U32 ic)
			: mIndex(vi), mCount(vc), mIndicesIndex(ii), mIndicesCount(ic)
		{ }	
	};

	std::vector<DirtyRegion> mDirtyRegions; //vector of dirty regions to rebuild

public:
	static BOOL sRenderActive;
	static S32 sCount;
	static S32 sGLCount;
	static std::vector<U32> sDeleteList;
	typedef std::list<LLVertexBuffer*> buffer_list_t;
	static buffer_list_t sLockedList;
	
	static BOOL sEnableVBOs;
	static S32 sTypeOffsets[TYPE_MAX];
	static U32 sGLRenderBuffer;
	static U32 sGLRenderIndices;
	static BOOL sVBOActive;
	static BOOL sIBOActive;
	static U32 sLastMask;
	static U32 sAllocatedBytes;
};


#endif // LL_LLVERTEXBUFFER_H
