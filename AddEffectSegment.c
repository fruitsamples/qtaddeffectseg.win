//////////
//
//	File:		AddEffectSegment.c
//
//	Contains:	Sample code for adding a visual effect to a segment of a movie.
//
//	Written by:	Tim Monroe
//
//	Copyright:	� 1999 by Apple Computer, Inc., all rights reserved.
//
//	Change History (most recent first):
//
//	   <2>	 	05/06/99	rtm		tested on Mac and Windows; works fine
//	   <1>	 	05/05/99	rtm		first file; some functions based on some existing code
//									in QTShowEffect
//	   
//	This file contains some sample code that adds a QuickTime effect to a part of a movie;
//	we assume that the movie already exists and that it contains one or more video tracks.
//	Our job here is to add a specified effect at a specified time. For one- or two-source
//	effects, the principal value in this code consists in illustrating how to duplicate the
//	portion(s) of the existing video track(s) for use as the effects source track(s).
//
//////////

#include "AddEffectSegment.h"


//////////
//
// QTEffSeg_AddEffectToMovieSegment
// Add the specified effect to occur at the specified time and duration.
//
//////////

OSErr QTEffSeg_AddEffectToMovieSegment (Movie theMovie, OSType theEffectType, long theNumSources, TimeValue theStartTime, TimeValue theDuration)
{
	Track					myVidTrack1 = NULL;
	Track					myVidTrack2 = NULL;
	Track					mySrcTrack1 = NULL;
	Track					mySrcTrack2 = NULL;
	Media					mySrcMedia1 = NULL;
	Media					mySrcMedia2 = NULL;
	Track					myEffectTrack = NULL;
	Media					myEffectMedia = NULL;
	Fixed					myWidth, myHeight;
	TimeScale				myTimeScale;
	TimeValue				mySampleTime;
	Rect					myRect;
	QTAtomContainer			myInputMap = NULL;
	QTAtomContainer			myEffectDesc = NULL;
	ImageDescriptionHandle	mySampleDesc = NULL;
	OSType					myEffectName1 = kSourceNoneName;
	OSType					myEffectName2 = kSourceNoneName;
	short					myLayer;
	OSErr					myErr = noErr;
	
	//////////
	//
	// get some information about the movie
	//
	//////////
	
	// make sure we were passed a valid movie
	if (theMovie == NULL)
		return(paramErr);
		
	myTimeScale = GetMovieTimeScale(theMovie);
	GetMovieBox(theMovie, &myRect);
	myLayer = QTEffSeg_GetFrontmostTrackLayer(theMovie, VideoMediaType);
	
	myWidth = (myRect.right - myRect.left) << 16;
	myHeight = (myRect.bottom - myRect.top) << 16;

	//////////
	//
	// retrieve the original video track(s), create the effect's source track(s) and media,
	// then set the new source track(s) to reference the data in the original video track(s)
	//
	//////////
	
	switch (theNumSources) {
		case 2:
			myVidTrack2 = GetMovieIndTrackType(theMovie, 2, VideoMediaType, movieTrackMediaType | movieTrackEnabledOnly);
			if (myVidTrack2 == NULL)
				return(paramErr);
			
			mySrcTrack2 = NewMovieTrack(theMovie, myWidth, myHeight, kNoVolume);
			if (mySrcTrack2 == NULL)
				return(paramErr);
				
			mySrcMedia2 = NewTrackMedia(mySrcTrack2, VideoMediaType, myTimeScale, NULL, 0);
			if (mySrcMedia2 == NULL)
				return(paramErr);

#if COPY_MOVIE_MEDIA
			myErr = BeginMediaEdits(mySrcMedia2);
			if (myErr != noErr)
				return(myErr);
#endif			
			myErr = CopyTrackSettings(myVidTrack2, mySrcTrack2);
			myErr = InsertTrackSegment(myVidTrack2, mySrcTrack2, theStartTime, theDuration, theStartTime);
			if (myErr != noErr)
				return(myErr);

#if COPY_MOVIE_MEDIA
			EndMediaEdits(mySrcMedia2);
#endif			
			myEffectName2 = kSourceTwoName;
			
			// note that we fall through here!
				
		case 1:
			myVidTrack1 = GetMovieIndTrackType(theMovie, 1, VideoMediaType, movieTrackMediaType | movieTrackEnabledOnly);
			if (myVidTrack1 == NULL)
				return(paramErr);
				
			mySrcTrack1 = NewMovieTrack(theMovie, myWidth, myHeight, kNoVolume);
			if (mySrcTrack1 == NULL)
				return(paramErr);
				
			mySrcMedia1 = NewTrackMedia(mySrcTrack1, VideoMediaType, myTimeScale, NULL, 0);
			if (mySrcMedia1 == NULL)
				return(paramErr);

#if COPY_MOVIE_MEDIA
			myErr = BeginMediaEdits(mySrcMedia1);
			if (myErr != noErr)
				return(myErr);
#endif			
			myErr = CopyTrackSettings(myVidTrack1, mySrcTrack1);
			myErr = InsertTrackSegment(myVidTrack1, mySrcTrack1, theStartTime, theDuration, theStartTime);
			if (myErr != noErr)
				return(myErr);
			
#if COPY_MOVIE_MEDIA
			EndMediaEdits(mySrcMedia1);
#endif			
			myEffectName1 = kSourceOneName;
			
			break;
			
		case 0:
			// for 0-source effects, we don't need to create any new source track
			break;
	
		default:
			return(paramErr);
	}
	
	//////////
	//
	// create the effects track and media
	//
	//////////

	myEffectTrack = NewMovieTrack(theMovie, myWidth, myHeight, kNoVolume);
	if (myEffectTrack == NULL)
		return(GetMoviesError());
		
	myEffectMedia = NewTrackMedia(myEffectTrack, VideoMediaType, myTimeScale, NULL, 0);
	if (myEffectMedia == NULL)
		return(GetMoviesError());
	
	// create an effect sample description
	mySampleDesc = QTEffSeg_MakeSampleDescription(theEffectType, myWidth >> 16, myHeight >> 16);
	if (mySampleDesc == NULL)
		goto bail;

	// create an effect description
	myEffectDesc = QTEffSeg_CreateEffectDescription(theEffectType, myEffectName1, myEffectName2);

	// add the effect description as a sample to the effect track media
	BeginMediaEdits(myEffectMedia);

	myErr = AddMediaSample(myEffectMedia, (Handle)myEffectDesc, 0, GetHandleSize((Handle)myEffectDesc), theDuration, (SampleDescriptionHandle)mySampleDesc, 1, 0, &mySampleTime);
	if (myErr != noErr)
		goto bail;

	EndMediaEdits(myEffectMedia);
	
	// add the media sample to the effects track
	myErr = InsertMediaIntoTrack(myEffectTrack, theStartTime, mySampleTime, theDuration, fixed1);
	if (myErr != noErr)
		goto bail;

	//////////
	//
	// create the input map and add references for the source track(s)
	//
	//////////

	myErr = QTNewAtomContainer(&myInputMap);
	if (myErr != noErr)
		goto bail;
	
	if (mySrcTrack1 != NULL) {	
		myErr = QTEffSeg_AddTrackReferenceToInputMap(myInputMap, myEffectTrack, mySrcTrack1, kSourceOneName);
		if (myErr != noErr)
			goto bail;
	}
		
	if (mySrcTrack2 != NULL) {	
		myErr = QTEffSeg_AddTrackReferenceToInputMap(myInputMap, myEffectTrack, mySrcTrack2, kSourceTwoName);
		if (myErr != noErr)
			goto bail;
	}
		
	// add the input map to the effects track
	myErr = SetMediaInputMap(myEffectMedia, myInputMap);
	if (myErr != noErr)
		goto bail;

	//////////
	//
	// do any required positioning and graphics mode manipulation
	//
	//////////

	SetTrackLayer(myEffectTrack, myLayer - 1);	// in front of any existing video track
	
	switch (theNumSources) {
		case 2:
			break;
			
		case 1:
			break;
			
		case 0: {
			RGBColor	myColor;
			
			myColor.red = 0;		// (good for fire, not so good for clouds)
			myColor.green = 0;
			myColor.blue = 0;
			
			MediaSetGraphicsMode(GetMediaHandler(myEffectMedia), transparent, &myColor);
			break;
		}
	}
	
bail:
	if (mySampleDesc != NULL)
		DisposeHandle((Handle)mySampleDesc);
	
	if (myInputMap != NULL)
		QTDisposeAtomContainer(myInputMap);
	
	return(myErr);
}


//////////
//
// QTEffSeg_CreateEffectDescription
// Create an effect description for zero, one, or two sources.
// 
// The effect description specifies which video effect is desired and the parameters for that effect.
// It also describes the source(s) for the effect. An effect description is simply an atom container
// that holds atoms with the appropriate information.
//
// Note that because we are creating an atom container, we must pass big-endian data (hence the calls
// to EndianU32_NtoB).
//
// The caller is responsible for disposing of the returned atom container, by calling QTDisposeAtomContainer.
//
//////////

QTAtomContainer QTEffSeg_CreateEffectDescription (OSType theEffectName, OSType theSourceName1, OSType theSourceName2)
{
	QTAtomContainer		myEffectDesc = NULL;
	OSType				myType;
	OSErr				myErr = noErr;

	// create a new, empty effect description
	myErr = QTNewAtomContainer(&myEffectDesc);
	if (myErr != noErr)
		goto bail;

	// create the effect ID atom: the atom type is kParameterWhatName, and the atom ID is kParameterWhatID
	myType = EndianU32_NtoB(theEffectName);
	myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, 0, sizeof(myType), &myType, NULL);
	if (myErr != noErr)
		goto bail;
		
	// add the first source, if it's not kSourceNoneName
	if (theSourceName1 != kSourceNoneName) {
		myType = EndianU32_NtoB(theSourceName1);
		myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kEffectSourceName, 1, 0, sizeof(myType), &myType, NULL);
		if (myErr != noErr)
			goto bail;
	}
							
	// add the second source, if it's not kSourceNoneName
	if (theSourceName2 != kSourceNoneName) {
		myType = EndianU32_NtoB(theSourceName2);
		myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kEffectSourceName, 2, 0, sizeof(myType), &myType, NULL);
	}

bail:
	return(myEffectDesc);
}


//////////
//
// QTEffSeg_AddTrackReferenceToInputMap
// Add a track reference to the specified input map.
// 
//////////

OSErr QTEffSeg_AddTrackReferenceToInputMap (QTAtomContainer theInputMap, Track theTrack, Track theSrcTrack, OSType theSrcName)
{
	OSErr				myErr = noErr;
	QTAtom				myInputAtom;
	long				myRefIndex;
	OSType				myType;

	myErr = AddTrackReference(theTrack, theSrcTrack, kTrackModifierReference, &myRefIndex);
	if (myErr != noErr)
		goto bail;
			
	// add a reference atom to the input map
	myErr = QTInsertChild(theInputMap, kParentAtomIsContainer, kTrackModifierInput, myRefIndex, 0, 0, NULL, &myInputAtom);
	if (myErr != noErr)
		goto bail;
	
	// add two child atoms to the parent reference atom
	myType = EndianU32_NtoB(kTrackModifierTypeImage);
	myErr = QTInsertChild(theInputMap, myInputAtom, kTrackModifierType, 1, 0, sizeof(myType), &myType, NULL);
	if (myErr != noErr)
		goto bail;
	
	myType = EndianU32_NtoB(theSrcName);
	myErr = QTInsertChild(theInputMap, myInputAtom, kEffectDataSourceType, 1, 0, sizeof(myType), &myType, NULL);
		
bail:
	return(myErr);
}
 
 
//////////
//
// QTEffSeg_MakeSampleDescription
// Return a new image description with default and specified values.
// 
//////////

ImageDescriptionHandle QTEffSeg_MakeSampleDescription (OSType theEffectType, short theWidth, short theHeight)
{
	ImageDescriptionHandle		mySampleDesc = NULL;

#if USES_MAKE_IMAGE_DESC_FOR_EFFECT
	OSErr						myErr = noErr;
	
	// create a new sample description
	myErr = MakeImageDescriptionForEffect(theEffectType, &mySampleDesc);
	if (myErr != noErr)
		return(NULL);
#else
	// create a new sample description
	mySampleDesc = (ImageDescriptionHandle)NewHandleClear(sizeof(ImageDescription));
	if (mySampleDesc == NULL)
		return(NULL);
		
	// fill in the fields of the sample description
	(**mySampleDesc).cType = theEffectType;
	(**mySampleDesc).idSize = sizeof(ImageDescription);
	(**mySampleDesc).hRes = 72L << 16;
	(**mySampleDesc).vRes = 72L << 16;
	(**mySampleDesc).frameCount = 1;
	(**mySampleDesc).depth = 0;
	(**mySampleDesc).clutID = -1;
#endif
	
	(**mySampleDesc).vendor = kAppleManufacturer;
	(**mySampleDesc).temporalQuality = codecNormalQuality;
	(**mySampleDesc).spatialQuality = codecNormalQuality;
	(**mySampleDesc).width = theWidth;
	(**mySampleDesc).height = theHeight;
	
	return(mySampleDesc);
}


//////////
//
// QTEffSeg_GetFrontmostTrackLayer
// Return the layer number of the frontmost track of the specified kind in a movie.
// 
//////////

short QTEffSeg_GetFrontmostTrackLayer (Movie theMovie, OSType theTrackType)
{
	short		myLayer = 0;
	short		myIndex = 1;
	Track		myTrack = NULL;
	
	// get the layer number of the first track of the specified kind;
	// if no track of that kind exists in the movie, return 0
	myTrack = GetMovieIndTrackType(theMovie, 1, theTrackType, movieTrackMediaType | movieTrackEnabledOnly);
	if (myTrack == NULL)
		return(myLayer);
		
	myLayer = GetTrackLayer(myTrack);
	
	// see if any of the remaining tracks have lower layer numbers
	while (myTrack != NULL) {
		if (myLayer > GetTrackLayer(myTrack))
			myLayer = GetTrackLayer(myTrack);
		myIndex++;
		myTrack = GetMovieIndTrackType(theMovie, myIndex, theTrackType, movieTrackMediaType | movieTrackEnabledOnly);
	}
	
	return(myLayer);
}


