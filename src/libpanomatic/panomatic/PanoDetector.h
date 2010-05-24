/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __detectpano_panodetector_h
#define __detectpano_panodetector_h

#include "PanoDetectorDefs.h"
#include <string>
#include <map>
#include <localfeatures/Image.h>
#include <localfeatures/PointMatch.h>
#include "TestCode.h"
#include <zthread/Runnable.h>
#include <zthread/PoolExecutor.h>

#include <localfeatures/KeyPoint.h>
#include <localfeatures/KeyPointDetector.h>
//REMOVING SURF #include <localfeatures/SurfKeyPointDescriptor.h>

class PanoDetector
{
public:
	typedef std::vector<std::string>						FileNameList_t;
	typedef std::vector<std::string>::iterator				FileNameListIt_t;
	typedef KDTreeSpace::KDTree<KDElemKeyPoint, double>		KPKDTree;
	typedef boost::shared_ptr<KPKDTree >					KPKDTreePtr;
	
	typedef lfeat::KeyPointDetector KeyPointDetector;
//REMOVING SURF 	typedef lfeat::SurfKeyPointDescriptor KeyPointDescriptor;
	
	PanoDetector();
	
	bool		checkData();
	void		printDetails();
	void		printHelp();
	void		run();
	
	
	// accessors
	inline void setGradientDescriptor(bool grad=true) { _gradDescriptor = grad; }
	inline bool getGradientDescriptor() const { return _gradDescriptor; }
	inline void setLoadKeypoints(bool loadKeypoints) { _loadKeypoints = loadKeypoints; }
	inline bool getLoadKeypoints() const { return _loadKeypoints; }

//REMOVING SURF 	inline void setSurfExtended(bool iExtended) { _extendedSurf = iExtended; }	
//REMOVING SURF 	inline void setSurfScoreThreshold(double iScore) { _surfScoreThreshold = iScore; }

//REMOVING SURF 	inline bool getSurfExtended() const { return _extendedSurf; }
//REMOVING SURF 	inline double getSurfScoreThreshold() const { return _surfScoreThreshold; }
	
	inline void setSieve1Width(int iWidth) { _sieve1Width = iWidth; }
	inline void setSieve1Height(int iHeight) { _sieve1Height = iHeight; }
	inline void setSieve1Size(int iSize) { _sieve1Size = iSize; }
	inline int  getSieve1Width() const { return _sieve1Width; }
	inline int  getSieve1Height() const { return _sieve1Height; }
	inline int  getSieve1Size() const { return _sieve1Size; }

	inline void setKDTreeSearchSteps(int iSteps) { _kdTreeSearchSteps = iSteps; }
	inline void setKDTreeSecondDistance(double iDist) { _kdTreeSecondDistance = iDist; }
	inline int  getKDTreeSearchSteps() const { return _kdTreeSearchSteps; }
	inline double  getKDTreeSecondDistance() const { return _kdTreeSecondDistance; }

	inline void setMinimumMatches(int iMatches) { _minimumMatches = iMatches; }
	inline void setRansacIterations(int iIters) { _ransacIters = iIters; }
	inline void setRansacDistanceThreshold(int iDT) { _ransacDistanceThres = iDT; }
	inline int  getMinimumMatches() const { return _minimumMatches; }
	inline int  getRansacIterations() const { return _ransacIters; }
	inline int  getRansacDistanceThreshold() const { return _ransacDistanceThres; }

	inline void setSieve2Width(int iWidth) { _sieve2Width = iWidth; }
	inline void setSieve2Height(int iHeight) { _sieve2Height = iHeight; }
	inline void setSieve2Size(int iSize) { _sieve2Size = iSize; }
	inline int  getSieve2Width() const { return _sieve2Width; }
	inline int  getSieve2Height() const { return _sieve2Height; }
	inline int  getSieve2Size() const { return _sieve2Size; }

	inline void setLinearMatch(bool iLin) { _linearMatch = iLin; }
	inline void setLinearMatchLen(int iLen) { _linearMatchLen = iLen; }
	inline bool getLinearMatch() const { return _linearMatch; }
	inline int  getLinearMatchLen() const { return _linearMatchLen; }

	inline bool	getDownscale() const { return _downscale; }
    inline void setDownscale(bool iDown) { _downscale = iDown; }

	
	inline void addFile(const std::string& iFile) { _files.push_back(iFile); }

	//	inline void setNumberOfKeys(int iNumKeys) { _numKeys = iNumKeys; }
	inline void setOutputFile(const std::string& iOutputFile) { _outputFile = iOutputFile; }
	inline void setTest(bool iTest) { _test = iTest; }
	inline bool getTest() const { return _test; }
	inline void setCores(int iCores) { _cores = iCores; }

	// predeclaration
	struct ImgData;
	struct MatchData;

private:
	// options
	
	bool					_loadKeypoints;
	// setup values			
//REMOVING SURF 	double					_surfScoreThreshold;
//REMOVING SURF 	bool					_extendedSurf;
	bool					_gradDescriptor;

	int						_sieve1Width;
	int						_sieve1Height;
	int						_sieve1Size;

	int						_kdTreeSearchSteps;
	double					_kdTreeSecondDistance;

	int						_minimumMatches;
	int						_ransacIters;
	int						_ransacDistanceThres;

	int						_sieve2Width;
	int						_sieve2Height;
	int						_sieve2Size;

	bool					_linearMatch;
	int						_linearMatchLen;

	bool					_test;
	int						_cores;
    bool                    _downscale;
	
	// list of files
	FileNameList_t			_files;
	std::string				_outputFile;

	// size of images
	
	void					prepareImages();
    bool                    checkLoadSuccess();
	void					prepareMatches();

	void					writeOutput();


	// internals
public:
	struct ImgData
	{	
		std::string		_name;
		std::string		_realImageName; ///< The real image name, differs from _name when loading keypoint files.
		int				_number;
		int				_detectWidth;
		int				_detectHeight;
		int				_origWidth;
		int				_origHeight;
		lfeat::Image	_ii;
		lfeat::KeyPointVect_t	_kp;
		int				_descLength;
        bool            _loadFail;

		// kdtree
		KDElemKeyPointVect_t	_kdv;
		KPKDTreePtr				_kd;
	};

	typedef std::map<std::string, ImgData>					ImgData_t;
	typedef std::map<std::string, ImgData>::iterator		ImgDataIt_t;

	struct MatchData
	{
		std::string				_i1_name;
		std::string				_i2_name;
		ImgData*				_i1;
		ImgData*				_i2;
		lfeat::PointMatchVector_t		_matches;
	};

	typedef std::vector<MatchData>								MatchData_t;
	typedef std::vector<MatchData>::iterator					MatchDataIt_t;

	// actions
	static bool				LoadKeypoints(ImgData & ioImgInfo, const PanoDetector& iPanoDetector);
	
	static bool				AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
	static bool				FindKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
	static bool				FilterKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
	static bool				MakeKeyPointDescriptorsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
	static bool				BuildKDTreesInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
	static bool				FreeMemoryInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);

	static bool				FindMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
	static bool				RansacMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
	static bool				FilterMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);

private:
	ImgData_t				_filesData;
	MatchData_t				_matchesData;

};



#endif // __detectpano_panodetector_h