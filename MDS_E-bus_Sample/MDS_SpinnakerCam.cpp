#include "MDS_SpinnakerCam.h"


MDS_SpinnakerCam::MDS_SpinnakerCam(int nIndex)
    : Manager(nullptr),
    m_nCamIndex(nIndex),
    m_Cam_Params(new Camera_Parameter)
{

}

MDS_SpinnakerCam::~MDS_SpinnakerCam()
{

}

ERRORCODE MDS_SpinnakerCam::CameraStart()
{

}

ERRORCODE MDS_SpinnakerCam::CameraStop()
{

}

ERRORCODE MDS_SpinnakerCam::IsConnect()
{

}

ERRORCODE MDS_SpinnakerCam::CameraDisconnect()
{

}
