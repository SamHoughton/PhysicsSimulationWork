#pragma once

#include "BasicActors.h"
#include <iostream>
#include <iomanip>

namespace PhysicsEngine
{
	using namespace std;

	//a list of colours: Circus Palette
	static const PxVec3 color_palette[] = {PxVec3(46.f/255.f,9.f/255.f,39.f/255.f),PxVec3(217.f/255.f,0.f/255.f,0.f/255.f),
		PxVec3(255.f/255.f,45.f/255.f,0.f/255.f),PxVec3(255.f/255.f,140.f/255.f,54.f/255.f),PxVec3(4.f/255.f,117.f/255.f,111.f/255.f), PxVec3(0.f/255.f ,209.f/255.f, 111.f/255.f), PxVec3(255.f / 255.f ,255.f / 255.f, 255.f / 255.f)};

	//pyramid vertices
	static PxVec3 pyramid_verts[] = {PxVec3(0,1,0), PxVec3(1,0,0), PxVec3(-1,0,0), PxVec3(0,0,1), PxVec3(0,0,-1)};
	//pyramid triangles: a list of three vertices for each triangle e.g. the first triangle consists of vertices 1, 4 and 0
	//vertices have to be specified in a counter-clockwise order to assure the correct shading in rendering
	static PxU32 pyramid_trigs[] = {1, 4, 0, 3, 1, 0, 2, 3, 0, 4, 2, 0, 3, 2, 1, 2, 4, 1};

	class Pyramid : public ConvexMesh
	{
	public:
		Pyramid(PxTransform pose=PxTransform(PxIdentity), PxReal density=1.f) :
			ConvexMesh(vector<PxVec3>(begin(pyramid_verts),end(pyramid_verts)), pose, density)
		{
		}
	};

	class PyramidStatic : public TriangleMesh
	{
	public:
		PyramidStatic(PxTransform pose=PxTransform(PxIdentity)) :
			TriangleMesh(vector<PxVec3>(begin(pyramid_verts),end(pyramid_verts)), vector<PxU32>(begin(pyramid_trigs),end(pyramid_trigs)), pose)
		{
		}
	};

	struct FilterGroup
	{
		enum Enum
		{
			ACTOR0		= (1 << 0),
			ACTOR1		= (1 << 1),
			ACTOR2		= (1 << 2)
			//add more if you need
		};
	};

	///A customised collision class, implemneting various callbacks
	class MySimulationEventCallback : public PxSimulationEventCallback
	{
	public:
		//an example variable that will be checked in the main simulation loop
		bool trigger;

		MySimulationEventCallback() : trigger(false) {}

		///Method called when the contact with the trigger object is detected.
		virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) 
		{
			//you can read the trigger information here
			for (PxU32 i = 0; i < count; i++)
			{
				//filter out contact with the planes
				if (pairs[i].otherShape->getGeometryType() != PxGeometryType::ePLANE)
				{
					//check if eNOTIFY_TOUCH_FOUND trigger
					if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
					{
						cerr << "onTrigger::eNOTIFY_TOUCH_FOUND" << endl;
						trigger = true;
					}
					//check if eNOTIFY_TOUCH_LOST trigger
					if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST)
					{
						cerr << "onTrigger::eNOTIFY_TOUCH_LOST" << endl;
						trigger = false;
					}
				}
			}
		}

		///Method called when the contact by the filter shader is detected.
		virtual void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs) 
		{
			cerr << "Contact found between " << pairHeader.actors[0]->getName() << " " << pairHeader.actors[1]->getName() << endl;

			//check all pairs
			for (PxU32 i = 0; i < nbPairs; i++)
			{
				//check eNOTIFY_TOUCH_FOUND
				if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
				{
					cerr << "onContact::eNOTIFY_TOUCH_FOUND" << endl;
				}
				//check eNOTIFY_TOUCH_LOST
				if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_LOST)
				{
					cerr << "onContact::eNOTIFY_TOUCH_LOST" << endl;
				}
			}
		}

		virtual void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) {}
		virtual void onWake(PxActor **actors, PxU32 count) {}
		virtual void onSleep(PxActor **actors, PxU32 count) {}
	};

	//A simple filter shader based on PxDefaultSimulationFilterShader - without group filtering
	static PxFilterFlags CustomFilterShader( PxFilterObjectAttributes attributes0,	PxFilterData filterData0,
		PxFilterObjectAttributes attributes1,	PxFilterData filterData1,
		PxPairFlags& pairFlags,	const void* constantBlock,	PxU32 constantBlockSize)
	{
		// let triggers through
		if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			return PxFilterFlags();
		}

		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		//enable continous collision detection
//		pairFlags |= PxPairFlag::eCCD_LINEAR;
		
		
		//customise collision filtering here
		//e.g.

		// trigger the contact callback for pairs (A,B) where 
		// the filtermask of A contains the ID of B and vice versa.
		if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		{
			//trigger onContact callback for this pair of objects
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
//			pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}

		return PxFilterFlags();
	};

	///Custom scene class
	class MyScene : public Scene
	{
		Plane* plane;
		Bottom* bottom, * bottom2;
		Top* top, *top2;
		Sphere* sphere1;
		Gun* gun1;
		Goal* goal1;
		MySimulationEventCallback* my_callback;
		
	public:
		//specify your custom filter shader here
		//PxDefaultSimulationFilterShader by default
		MyScene() : Scene() {};

		///A custom scene class
		void SetVisualisation()
		{
			px_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
			px_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
			px_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
			px_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
	
		}

		//Custom scene initialisation
		virtual void CustomInit() 
		{
			SetVisualisation();			

			GetMaterial()->setDynamicFriction(.2f);

			///Initialise and set the customised event callback
			my_callback = new MySimulationEventCallback();
			px_scene->setSimulationEventCallback(my_callback);

			plane = new Plane();
			plane->Color(color_palette[5]);
			Add(plane);

			bottom = new Bottom(PxTransform(PxVec3(.0f,.5f,30.0f)));
			bottom->Color(color_palette[6]);

			bottom2 = new Bottom(PxTransform(PxVec3(.0f, .5f, -30.0f)));
			bottom2->Color(color_palette[6]);
			
			top = new Top(PxTransform(PxVec3(30.0f, 0.5f, 0.0f)));
			top->Color(color_palette[6]);

			sphere1 = new Sphere(PxTransform(PxVec3(.5f, 2.f, 2.f)));
			sphere1->Color(color_palette[0]);

			gun1 = new Gun(PxTransform(PxVec3(-15.f, 2.f, 0.f)));
			gun1->Color(color_palette[4]);

			goal1 = new Goal(PxTransform(PxVec3(15.f, 0.f, 0.f), PxQuat(PxPiDivTwo, PxVec3(0.f,1.f,0.f))));
			goal1->Color(color_palette[4]);

			//set collision filter flags
			bottom->SetupFiltering(FilterGroup::ACTOR0, FilterGroup::ACTOR1);

			bottom2->SetupFiltering(FilterGroup::ACTOR0, FilterGroup::ACTOR1);

			top->SetupFiltering(FilterGroup::ACTOR0, FilterGroup::ACTOR1);
			
			bottom->Name("Bottom1");
			bottom2->Name("Bottom2");
			top->Name("Box3");
			
			Add(bottom);
			Add(bottom2);
			Add(top);
			Add(sphere1);
			Add(goal1);
			Add(gun1);


			DistanceJoint joint(bottom, PxTransform(PxVec3(0.f, 0.f, 0.f), PxQuat(PxPi / 2, PxVec3(0.f, 1.f, 0.f))), top, PxTransform(PxVec3(0.f, 5.f, 0.f)));
			DistanceJoint joint2(bottom2, PxTransform(PxVec3(0.f, 0.f, 0.f), PxQuat(PxPi / 2, PxVec3(0.f, 1.f, 0.f))), top, PxTransform(PxVec3(0.f, 5.f, 0.f)));



		}

		//Custom udpate function
		virtual void CustomUpdate() 
		{

		}


		/// An example use of key release handling
		void GunShot()
		{
			sphere1 = new Sphere(PxTransform(PxVec3(-12.f, 2.f, 0.f)));
			sphere1->Color(color_palette[0]);
			Add(sphere1);
			
			//get the actor
			PxActor* actor = sphere1->Get();

			if (actor->isRigidBody())
			{
				//actor is 100% a rigidbody
				//..

				//get the rigidbody by casting to it
				
				PxRigidBody* rigidbody = (PxRigidBody*)actor;
				
				

				//add some forces
				rigidbody->addForce(PxVec3(0.5f, .0f, .0f), PxForceMode::eIMPULSE, 1);
			}
			
			//PxRigidBody::addForce(PxVec3)
			
			//PxRigidBody::addForce(*(PxRigidDynamic*)sphere1, 1.0f);
			
		}

		void shotGun() 
		{
			int i = 0;
			int x = 0;
			
			while (i < 5)
			{
				PxActor* GunActor = gun1->Get();

				if (GunActor->isRigidBody()) {
					
					PxRigidBody* rigidbodyGun = (PxRigidBody*)GunActor;
					PxTransform pose = rigidbodyGun->getGlobalPose();
					sphere1 = new Sphere(PxTransform(pose));
					sphere1->Color(color_palette[2]);
					Add(sphere1);
				}

				PxActor* BallActor = sphere1->Get();								

			if (BallActor->isRigidBody())
			{
				//actor is 100% a rigidbody
				//..

				//get the rigidbody by casting to it

				PxRigidBody* rigidbodyBall = (PxRigidBody*)BallActor;

				float max = 1;
				int min = 0.2;

				float Px = rand() % 2;
				float Py = rand() % 1;
				int Pz = rand() % 1;

				//add some forces
				rigidbodyBall ->addForce(PxVec3(Px, Py, .0f), PxForceMode::eIMPULSE, 1);
			}

				x++;
				i++;
			}

			
			//PxRigidBody::addForce(PxVec3)

			//PxRigidBody::addForce(*(PxRigidDynamic*)sphere1, 1.0f);
		}

		void StartGame()
		{
			int Px = 0;

			PxActor* actor1 = gun1->Get();

			if (actor1->isRigidBody())
			{
				PxRigidBody* rigidbody1 = (PxRigidBody*)actor1;

				while (Px < 10) {

					rigidbody1->addForce(PxVec3(0.f, 0.0f, Px), PxForceMode::eIMPULSE, 1);
					Px++;
				}
			}
		}

		/// An example use of key presse handling
		void ExampleKeyPressHandler()
		{
			cerr << "I am pressed!" << endl;
		}
	};
}
