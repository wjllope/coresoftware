#include "AlignmentTransformation.h"

#include "ActsGeometry.h"
#include "TpcDefs.h"
#include "TrkrDefs.h"

#include <ffamodules/XploadInterface.h>

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>
#include <phool/phool.h>
#include <phool/PHDataNode.h>
#include <phool/PHNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>
#include <phool/PHTimer.h>

#include <Acts/Surfaces/PerigeeSurface.hpp>
#include <Acts/Surfaces/PlaneSurface.hpp>
#include <Acts/Surfaces/Surface.hpp>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/LU>

#include <cmath>
#include <fstream>



void AlignmentTransformation::createMap(PHCompositeNode* topNode)
{ 
  localVerbosity = 0;

  getNodes(topNode);

 // Use construction transforms as a reference for making the map
 if(alignmentTransformationContainer::use_alignment) alignmentTransformationContainer::use_alignment = false;

 // Define Parsing Variables
 TrkrDefs::hitsetkey hitsetkey = 0;
 float alpha = 0.0, beta = 0.0, gamma = 0.0, dx = 0.0, dy = 0.0, dz = 0.0;

 // load alignment constants file
 std::ifstream datafile;
 datafile.open(alignmentParamsFile);  //  looks for default file name on disk
 if(datafile.is_open())
   {
     std::cout << "AlignmentTransformation: Reading alignment parameters from disk file: "
	       << alignmentParamsFile << " localVerbosity = " << localVerbosity << std::endl; 
   }
 else
   { 
     datafile.clear();
     // load alignment constants file from database
     alignmentParamsFile = XploadInterface::instance()->getUrl("TRACKINGALIGNMENT");
     std::cout << "AlignmentTransformation: Reading alignment parameters from database file: " << alignmentParamsFile << std::endl; 
     datafile.open(alignmentParamsFile);
   }
 
 ActsSurfaceMaps surfMaps = m_tGeometry->maps();
 Surface surf;

 int fileLines = 1824;
 for (int i=0; i<fileLines; i++)
   {
     datafile >> hitsetkey >> alpha >> beta >> gamma >> dx >> dy >> dz;
     
     // Perturbation translations and angles for stave and sensor
     Eigen::Vector3d sensorAngles(alpha,beta,gamma);  
     Eigen::Vector3d millepedeTranslation(dx,dy,dz); 

     unsigned int trkrId = TrkrDefs::getTrkrId(hitsetkey); // specify between detectors


     perturbationAngles      = Eigen::Vector3d(0.0,0.0,0.0);
     perturbationTranslation = Eigen::Vector3d(0.0,0.0,0.0);

     if(trkrId == TrkrDefs::mvtxId) 
       {
	 if(perturbMVTX)
	   {
	     generateRandomPerturbations(mvtxAngleDev, mvtxTransDev);
	     sensorAngles         = sensorAngles + perturbationAngles;
	     millepedeTranslation = millepedeTranslation + perturbationTranslation;
	   }

         surf                        = surfMaps.getSiliconSurface(hitsetkey);
	 Acts::Transform3 transform  = newMakeTransform(surf, millepedeTranslation, sensorAngles);
         Acts::GeometryIdentifier id = surf->geometryId();

	 if(localVerbosity) 
	   {
	     //Acts::Transform3 transform2  = makeTransform(surf, millepedeTranslation, sensorAngles);
	     std::cout << " Add transform for MVTX with surface GeometryIdentifier " << id << " trkrid " << trkrId << std::endl;
	     std::cout << " final mvtx transform:" << std::endl << transform.matrix() << std::endl;
	     //	     std::cout << "mvtx transform2:" << std::endl << transform2.matrix() << std::endl;
	   }
	 transformMap->addTransform(id,transform);
       }
     
     else if(trkrId == TrkrDefs::inttId) 
       {

	 if(perturbINTT)
	   {
	     generateRandomPerturbations(inttAngleDev,inttTransDev);
	     sensorAngles         = sensorAngles + perturbationAngles;
	     millepedeTranslation = millepedeTranslation + perturbationTranslation;
	   }

         surf                        = surfMaps.getSiliconSurface(hitsetkey);
	 //Acts::Transform3 transform  = makeTransform(surf, millepedeTranslation, sensorAngles);
	 Acts::Transform3 transform  = newMakeTransform(surf, millepedeTranslation, sensorAngles);
         Acts::GeometryIdentifier id = surf->geometryId();

	 if(localVerbosity) 
	   {
	     std::cout << " Add transform for INTT with surface GeometryIdentifier " << id << " trkrid " << trkrId << std::endl;
	   }

	 transformMap->addTransform(id,transform);
       }


     else if(trkrId == TrkrDefs::tpcId)
       {
	 if(perturbTPC)
	   {
	     generateRandomPerturbations(tpcAngleDev,tpcTransDev);	 
	     sensorAngles         = sensorAngles + perturbationAngles;
	     millepedeTranslation = millepedeTranslation + perturbationTranslation;
	   }
	 unsigned int sector         = TpcDefs::getSectorId(hitsetkey);
	 unsigned int side           = TpcDefs::getSide(hitsetkey);
	 unsigned int subsurfkey_min = sector * 12 + (1-side) * 144;
	 unsigned int subsurfkey_max = subsurfkey_min + 12;

	 for(unsigned int subsurfkey = subsurfkey_min; subsurfkey<subsurfkey_max; subsurfkey++)
	   {
             surf                        = surfMaps.getTpcSurface(hitsetkey,subsurfkey);
	     //Acts::Transform3 transform  = makeTransform(surf, millepedeTranslation, sensorAngles);
	     Acts::Transform3 transform  = newMakeTransform(surf, millepedeTranslation, sensorAngles);
             Acts::GeometryIdentifier id = surf->geometryId();

	     if(localVerbosity) 
	       {
		 std::cout << " Add transform for TPC with surface GeometryIdentifier " << id << " trkrid " << trkrId << std::endl;
	       }
	     transformMap->addTransform(id,transform);
	   }
       }
     else if(trkrId == TrkrDefs::micromegasId)
      {
	if(perturbMM)
	  {
	    generateRandomPerturbations(mmAngleDev,mmTransDev);

	     sensorAngles         = sensorAngles + perturbationAngles;
	     millepedeTranslation = millepedeTranslation + perturbationTranslation;
	  }
	surf                        = surfMaps.getMMSurface(hitsetkey);
	//Acts::Transform3 transform  = makeTransform(surf, millepedeTranslation, sensorAngles);
	Acts::Transform3 transform  = newMakeTransform(surf, millepedeTranslation, sensorAngles);
	Acts::GeometryIdentifier id = surf->geometryId();

	if(localVerbosity)
	  { 
	    std::cout << " Add transform for Micromegas with surface GeometryIdentifier " << id << " trkrid " << trkrId << std::endl;
	  }

	transformMap->addTransform(id,transform);
      }

     else
       {
	 std::cout<< "Error: Invalid Hitsetkey" << std::endl;
       }
   } 

 // copy map into geoContext
 m_tGeometry->geometry().geoContext =  transformMap;

 // map is created, now we can use the transforms
 alignmentTransformationContainer::use_alignment = true;

 
}

// no longer used
Eigen::Matrix3d AlignmentTransformation::rotateToGlobal(Surface surf)
{  
  /*
    Get ideal geometry rotation, by aligning surface to surface normal vector in global coordinates
    URL: https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d
  */  
 
  Eigen::Vector3d ylocal(0,1,0);
  Eigen::Vector3d sensorNormal    = -surf->normal(m_tGeometry->geometry().getGeoContext());
  sensorNormal                    = sensorNormal/sensorNormal.norm(); // make unit vector 
  double cosTheta                 = ylocal.dot(sensorNormal);
  double sinTheta                 = (ylocal.cross(sensorNormal)).norm();
  Eigen::Vector3d vectorRejection = (sensorNormal - (ylocal.dot(sensorNormal))*ylocal)/(sensorNormal - (ylocal.dot(sensorNormal))*ylocal).norm();
  Eigen::Vector3d perpVector      =  sensorNormal.cross(ylocal);

  // Initialize and fill matrices (row,col)
  Eigen::Matrix3d fInverse;
  fInverse(0,0) = ylocal(0);
  fInverse(1,0) = ylocal(1);
  fInverse(2,0) = ylocal(2);
  fInverse(0,1) = vectorRejection(0);
  fInverse(1,1) = vectorRejection(1); 
  fInverse(2,1) = vectorRejection(2);
  fInverse(0,2) = perpVector(0);
  fInverse(1,2) = perpVector(1);
  fInverse(2,2) = perpVector(2);
  
  Eigen::Matrix3d G;
  G(0,0) =  cosTheta;
  G(0,1) = -sinTheta;
  G(0,2) =  0;
  G(1,0) =  sinTheta;
  G(1,1) =  cosTheta;
  G(1,2) =  0;
  G(2,0) =  0;
  G(2,1) =  0;
  G(2,2) =  1;

  Eigen::Matrix3d globalRotation = fInverse * G * (fInverse.inverse()); 

  if(localVerbosity > 2)
    {
      std::cout<< " global rotation: "<< std::endl << globalRotation <<std::endl;
    }
  return globalRotation;
}

// no longer used
Eigen::Matrix3d AlignmentTransformation::modifyRotationConvention(Eigen::Matrix3d rotationMatrix)
{
  // Acts uses a rotation matrix that transforms local position (x,z,y) into global position (x',y',z')
  // That rotation matrix is obtained from the one we have (which does (x,y,z) to (x',y',z') by:
  //    exchanging column 2 and 3 (y with z)
  //    flipping the signs of the original content of column 2

  Eigen::Matrix3d actsRotationMatrix;
  actsRotationMatrix(0,0) = rotationMatrix(0,0);
  actsRotationMatrix(1,0) = rotationMatrix(1,0);
  actsRotationMatrix(2,0) = rotationMatrix(2,0);

  // flip column 1 and 2
  actsRotationMatrix(0,1) = rotationMatrix(0,2);
  actsRotationMatrix(1,1) = rotationMatrix(1,2);
  actsRotationMatrix(2,1) = rotationMatrix(2,2);

  actsRotationMatrix(0,2) = -rotationMatrix(0,1);
  actsRotationMatrix(1,2) = -rotationMatrix(1,1);
  actsRotationMatrix(2,2) = -rotationMatrix(2,1);

  return actsRotationMatrix;
}

// no longer used
Acts::Transform3 AlignmentTransformation::makeAffineMatrix(Eigen::Matrix3d rotationMatrix, Eigen::Vector3d translationVector)
{
  // Acts uses a rotation matrix that transforms local position (x,z,y) into global position (x',y',z')
  // That rotation matrix is obtained from the one we have (which does (x,y,z) to (x',y',z') by:
  //    exchanging column 2 and 3 (y with z)
  //    flipping the signs of the original content of column 2

  Eigen::Matrix3d actsRotationMatrix;
  actsRotationMatrix(0,0) = rotationMatrix(0,0);
  actsRotationMatrix(1,0) = rotationMatrix(1,0);
  actsRotationMatrix(2,0) = rotationMatrix(2,0);

  // flip column 1 and 2
  actsRotationMatrix(0,1) = rotationMatrix(0,2);
  actsRotationMatrix(1,1) = rotationMatrix(1,2);
  actsRotationMatrix(2,1) = rotationMatrix(2,2);

  actsRotationMatrix(0,2) = -rotationMatrix(0,1);
  actsRotationMatrix(1,2) = -rotationMatrix(1,1);
  actsRotationMatrix(2,2) = -rotationMatrix(2,1);

  // Creates 4x4 affine matrix given rotation matrix and translationVector 
  Acts::Transform3 affineMatrix;
  affineMatrix.linear() = actsRotationMatrix;
  affineMatrix.translation() = translationVector;
  return affineMatrix;
}

Acts::Transform3 AlignmentTransformation::newMakeTransform(Surface surf, Eigen::Vector3d millepedeTranslation, Eigen::Vector3d sensorAngles)
{
  //define null matrices
  Eigen::Vector3d nullTranslation(0,0,0);
  Eigen::AngleAxisd a(0, Eigen::Vector3d::UnitX());
  Eigen::AngleAxisd b(0, Eigen::Vector3d::UnitY());
  Eigen::AngleAxisd g(0, Eigen::Vector3d::UnitZ());
  Eigen::Quaternion<double> qnull       = g*b*a;
  Eigen::Matrix3d nullRotation = qnull.matrix();

  // Create alignment rotation matrix

  // Note that Acts transforms local coordinates of (x,z,y) to global (x,y,z)
  //=====================================================
  // So our beginning local position vector should be (x,z,y)
  // and our MP (local) alignment translation vector should be (dx,dz,dy)
  // We don't worry about the angle parameter order, since they are 
  // just arbitrary parameters that will be fitted to data
  //=====================================================

  Eigen::AngleAxisd alpha(sensorAngles(0), Eigen::Vector3d::UnitX());
  Eigen::AngleAxisd beta(sensorAngles(1), Eigen::Vector3d::UnitY());
  Eigen::AngleAxisd gamma(sensorAngles(2), Eigen::Vector3d::UnitZ());
  Eigen::Quaternion<double> q       = gamma*beta*alpha;
  Eigen::Matrix3d millepedeRotation = q.matrix();

  Acts::Transform3 mpRotationAffine;   
  mpRotationAffine.linear() = millepedeRotation;   
  mpRotationAffine.translation() = nullTranslation;   

  // create alignment translation matrix
  Acts::Transform3 mpTranslationAffine;   
  mpTranslationAffine.linear() = nullRotation;   
  mpTranslationAffine.translation() = millepedeTranslation;   

  // get the acts transform components
  Acts::Transform3 actsTransform = surf->transform(m_tGeometry->geometry().getGeoContext());
  Eigen::Matrix3d actsRotationPart    = actsTransform.rotation();
  Eigen::Vector3d actsTranslationPart    = actsTransform.translation();

  // and make affine matrices from each
  Acts::Transform3 actsRotationAffine;
  actsRotationAffine.linear() = actsRotationPart;
  actsRotationAffine.translation() = nullTranslation;
  Acts::Transform3 actsTranslationAffine;
  actsTranslationAffine.linear() = nullRotation;
  actsTranslationAffine.translation() = actsTranslationPart;

  //Put them together into a combined transform

  // EITHER: put the mp translations in the global frame
  // Acts::Transform3 transform = mpTranslationAffine *  actsTranslationAffine *  actsRotationAffine * mpRotationAffine;
  // OR (new): put the mp translations in the local coordinate frame
  Acts::Transform3 transform =  actsTranslationAffine *  actsRotationAffine * mpTranslationAffine * mpRotationAffine;

  if(localVerbosity > 2)
    {
      std::cout << "newMakeTransform" << std::endl;
      std::cout << "mpRotationAffine: "<< std::endl<< mpRotationAffine.matrix()  <<std::endl;
      std::cout << "mpTranslationAffine: " << std::endl << mpTranslationAffine.matrix() <<std::endl;
      std::cout << " mptranslationAffine * mpRotationAffine " << std::endl << (mpTranslationAffine * mpRotationAffine).matrix() << std::endl;
      std::cout << "actsRotationAffine: "<< std::endl<< actsRotationAffine.matrix()  <<std::endl;
      std::cout << "actsTranslationAffine: "<< std::endl<< actsTranslationAffine.matrix()  <<std::endl;
      std::cout << "Overall transform: " << std::endl << transform.matrix() <<std::endl;
    }

  return transform;   
}

// no longer used
Acts::Transform3 AlignmentTransformation::makeTransform(Surface surf, Eigen::Vector3d millepedeTranslation, Eigen::Vector3d sensorAngles)
{
  // Create alignment rotation matrix
  Eigen::AngleAxisd alpha(sensorAngles(0), Eigen::Vector3d::UnitX());
  Eigen::AngleAxisd beta(sensorAngles(1), Eigen::Vector3d::UnitY());
  Eigen::AngleAxisd gamma(sensorAngles(2), Eigen::Vector3d::UnitZ());
  Eigen::Quaternion<double> q       = gamma*beta*alpha;
  Eigen::Matrix3d millepedeRotation = q.matrix();

  // Create ideal rotation matrix from ActsGeometry
  Eigen::Matrix3d globalRotation    = AlignmentTransformation::rotateToGlobal(surf);
  Eigen::Matrix3d combinedRotation  = globalRotation * millepedeRotation; 
  Eigen::Vector3d sensorCenter      = surf->center(m_tGeometry->geometry().getGeoContext());//*0.1;
  Eigen::Vector3d globalTranslation = sensorCenter + millepedeTranslation;
  Acts::Transform3 transformation   = AlignmentTransformation::makeAffineMatrix(combinedRotation,globalTranslation);

  if(localVerbosity > 2)
    {
      std::cout << "makeTransform:" << std::endl;
      std::cout << "sensor center: " << std::endl << sensorCenter << std::endl << " millepede translation: " << std::endl << millepedeTranslation <<std::endl;
      std::cout << "transformation: "<< std::endl<< transformation.matrix()  <<std::endl;
    }

  return transformation;   
}


int AlignmentTransformation::getNodes(PHCompositeNode* topNode)
{
  m_tGeometry = findNode::getClass<ActsGeometry>(topNode, "ActsGeometry");
  if(!m_tGeometry)
    {
      std::cout << "ActsGeometry not on node tree. Exiting."
		<< std::endl;
      
      return Fun4AllReturnCodes::ABORTEVENT;
    }

  return 0; 
}

void AlignmentTransformation::misalignmentFactor(TrkrDefs::TrkrId id, const double factor)
{
  transformMap->setMisalignmentFactor(id, factor);
}
void AlignmentTransformation::createAlignmentTransformContainer(PHCompositeNode* topNode)
{
  //​ Get a pointer to the top of the node tree
  PHNodeIterator iter(topNode);
 
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
    {
      std::cerr << "DST node is missing, quitting" << std::endl;
      throw std::runtime_error("Failed to find DST node in AlignmentTransformation::createNodes");
    }

  transformMap = findNode::getClass<alignmentTransformationContainer>(topNode, "alignmentTransformationContainer");
  if(!transformMap)
    {
      transformMap = new alignmentTransformationContainer;
      auto node    = new PHDataNode<alignmentTransformationContainer>(transformMap, "alignmentTransformationContainer");
      dstNode->addNode(node);
    }
}


void AlignmentTransformation::generateRandomPerturbations(Eigen::Vector3d angleDev, Eigen::Vector3d transformDev)
{
  /*Creates random perturbations for the correctional parameters with a given standard deviation and mean of zero*/

  std::cout << "Generating Random Perturbations..."<<std::endl;

  if(angleDev(0)!=0)
    {
      std::normal_distribution<double> distribution(0,angleDev(0));
      perturbationAngles(0) = distribution(generator);
    }
  if(angleDev(1)!=0)
    {
      std::normal_distribution<double> distribution(0,angleDev(1));
      perturbationAngles(1) = distribution(generator);
    }
  if(angleDev(2)!=0)
    {
      std::normal_distribution<double> distribution(0,angleDev(2));
      perturbationAngles(2) = distribution(generator);
    }
  if(transformDev(0)!=0)
    {
      std::normal_distribution<double> distribution(0,transformDev(0));
      perturbationTranslation(0) = distribution(generator);
    }
  if(transformDev(1)!=0)
    {
      std::normal_distribution<double> distribution(0,transformDev(1));
      perturbationTranslation(1) = distribution(generator);
    }
  if(transformDev(2)!=0)
    {
      std::normal_distribution<double> distribution(0,transformDev(2));
      perturbationTranslation(2) = distribution(generator);
    }
  if(localVerbosity)
    {
      std::cout << "randomperturbationAngles" << perturbationAngles << " randomperturbationTrans:" << perturbationTranslation << std::endl;
    }
}

