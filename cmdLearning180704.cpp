#include "StdAfx.h"
#include "Learning180704PlugIn.h"


#pragma region Learning180704 command


class CCommandLearning180704 : public CRhinoCommand
{
public:
	
	CCommandLearning180704() = default;
	~CCommandLearning180704() = default;


	UUID CommandUUID() override
	{
		// {F2449112-EABD-4E7B-A9F3-38509AE49367}
		static const GUID Learning180704Command_UUID =
		{ 0xF2449112, 0xEABD, 0x4E7B, { 0xA9, 0xF3, 0x38, 0x50, 0x9A, 0xE4, 0x93, 0x67 } };
		return Learning180704Command_UUID;
	}

	const wchar_t* EnglishCommandName() override { return L"Petras_PlaneSplit"; }

	CRhinoCommand::result RunCommand(const CRhinoCommandContext& context) override;
};


static class CCommandLearning180704 theLearning180704Command;

const ON_LineCurve* GetLineCurve(const ON_Curve* crv) {
	const ON_LineCurve* p = 0;
	if (crv != 0)
		p = ON_LineCurve::Cast(crv);
	return p;
}

const ON_ArcCurve* GetArcCurve(const ON_Curve* crv) {
	const ON_ArcCurve* p = 0;
	if (crv != 0)
		p = ON_ArcCurve::Cast(crv);
	return p;
}

const ON_PolylineCurve* GetPolylineCurve(const ON_Curve* crv)
{
	const ON_PolylineCurve* p = 0;
	if (crv != 0)
		p = ON_PolylineCurve::Cast(crv);
	return p;
}

const ON_Polyline GetPolyline(const ON_Curve* crv)
{
	const ON_PolylineCurve* p = 0;
	if (crv != 0)
		p = ON_PolylineCurve::Cast(crv);
	return p->m_pline;
}

const ON_PolyCurve* GetPolyCurve(const ON_Curve* crv)
{
	const ON_PolyCurve* p = 0;
	if (crv != 0)
		p = ON_PolyCurve::Cast(crv);
	return p;
}

const ON_NurbsCurve* GetNurbsCurve(const ON_Curve* crv) {
	const ON_NurbsCurve* p = 0;
	if (crv != 0)
		p = ON_NurbsCurve::Cast(crv);
	return p;
}

bool RhinoBrepSplitPlane(
	const ON_Brep& brep,
	const ON_Plane& plane,
	double tolerance,
	ON_SimpleArray<ON_Brep*>& pieces,
	bool* bRaisedTol = nullptr
) {
	auto bbox = brep.BoundingBox();
	double length = bbox.Diagonal().Length();

	RhinoApp().Print(L"Length %d.\n", length);
	ON_Interval d0, d1;
	d0.Set(-length, length);
	d1.Set(-length, length);

	ON_PlaneSurface* srf = new ON_PlaneSurface(plane);
	srf->SetExtents(0, d0, true);
	srf->SetExtents(1, d1, true);
	srf->SetDomain(0, d0.Min(), d0.Max());
	srf->SetDomain(1, d1.Min(), d1.Max());

	RhinoApp().Print(L"SurfaceValid %d.\n", srf->IsValid());



	ON_Brep splitter;
	splitter.Create(srf);//splitter will delete srf
	RhinoApp().Print(L"BrepValid %d.\n", splitter.IsValid());

	return RhinoBrepSplit(brep, splitter, tolerance, pieces);//bRaisedTol
}

bool RhinoBrepSplitRectangle(
	const ON_Brep& brep,
	const ON_Curve*& curve,
	double tolerance,
	ON_SimpleArray<ON_Brep*>& pieces,
	const CRhinoCommandContext& context,
	bool* bRaisedTol = nullptr
	
) {

	//Try get Polyline
	ON_SimpleArray< ON_3dPoint > points;
	int n = curve->IsPolyline(&points);

	//If curve it is rectangle
	if (n == 5) {

		//Get Center of curve
		ON_3dPoint basePt(0, 0, 0);
		for (int i = 0; i < n - 1; i++)
			basePt += points[i];
		basePt /= n - 1;

		//Get xdir and ydir of curve
		ON_3dVector vec0 = points[0] - points[1];
		ON_3dVector vec1 = points[0] - points[n - 2];
		vec0.Unitize();
		vec1.Unitize();

		//Construct plane
		const ON_Plane cutPlane(basePt, vec0, vec1);

		//Get brep diagional length and construct plane surface from that length
		auto bbox = brep.BoundingBox();
		double length = bbox.Diagonal().Length();

		ON_Interval d0, d1;
		d0.Set(-length, length);
		d1.Set(-length, length);

		ON_PlaneSurface* srf = new ON_PlaneSurface(cutPlane);
		srf->SetExtents(0, d0, true);
		srf->SetExtents(1, d1, true);
		srf->SetDomain(0, d0.Min(), d0.Max());
		srf->SetDomain(1, d1.Min(), d1.Max());

		//Convert ON_PlaneSurface to brep
		ON_Brep splitter;
		splitter.Create(srf);//splitter will delete srf

		//RhinoApp().Print(L"BrepValid %d.\n", splitter.IsValid());
		//Split pieces by reference, references pieces will be modified
		bool flag0 = RhinoBrepSplit(brep, splitter, tolerance, pieces);//bRaisedTol

		//Rotate surface 90 degrees
		//context.m_doc.AddBrepObject(splitter);
		//context.m_doc.AddPointObject(cutPlane.Origin());
		//context.m_doc.AddPointObject(cutPlane.Origin()+ cutPlane.xaxis);
		splitter.Rotate(90*ON_PI /180,  cutPlane.xaxis, cutPlane.Origin());
		//context.m_doc.AddBrepObject(splitter);

		bool flag1 = RhinoBrepSplit(*pieces[1], splitter, tolerance, pieces);//bRaisedTol
		bool flag2 = RhinoBrepSplit(*pieces[0], splitter, tolerance, pieces);//bRaisedTol

		pieces[2] = pieces[3]->Duplicate();
		ON_Xform xform;
		xform.Mirror(cutPlane.Origin(), cutPlane.zaxis);
		pieces[2]->Transform(xform);


		pieces[4] = pieces[2]->Duplicate();
		ON_Xform xform1;
		xform1.Mirror(cutPlane.Origin(), cutPlane.yaxis);
		pieces[4]->Transform(xform1);

		pieces[5] = pieces[3]->Duplicate();
		ON_Xform xform2;
		xform2.Mirror(cutPlane.Origin(), cutPlane.yaxis);
		pieces[5]->Transform(xform2);


		return flag1;
	}
	else return false;
}

CRhinoCommand::result CCommandLearning180704::RunCommand(const CRhinoCommandContext& context)
{
	//---------------------------------Document tolerance
	double tol = context.m_doc.AbsoluteTolerance();

	//---------------------------------Select Rectangle
	//Get Geometry
	CRhinoGetObject g;
	g.SetCommandPrompt(L"Select Rectangle");
	g.SetGeometryFilter(CRhinoGetObject::curve_object);
	g.GetObjects(1, 1);

	//Check if it good
	if (g.CommandResult() != CRhinoCommand::success)
		return g.CommandResult();

	//Get first brep out of all breps
	const ON_Curve* curve = g.Object(0).Curve();

	//Check if it is good
	if (curve == 0)
		return CRhinoCommand::failure;



	//---------------------------------Select 1 Brep to split https://developer.rhino3d.com/samples/cpp/split-brep/
	CRhinoGetObject go;
	go.SetCommandPrompt(L"Select surface or polysuface to split");
	go.SetGeometryFilter(CRhinoGetObject::surface_object | CRhinoGetObject::polysrf_object);
	go.GetObjects(1, 1);
	if (go.CommandResult() != success)
		return go.CommandResult();

	//Get first rhino object
	const CRhinoObjRef& split_ref = go.Object(0);

	//COnvert reference object to pointer
	const CRhinoObject* split_object = split_ref.Object();
	if (!split_object)
		return failure;

	//Convert CRhinoObject to Brep
	const ON_Brep* split = split_ref.Brep();
	if (!split)
		return failure;



	//---------------------------------Split brep and retrieve pieces
	ON_SimpleArray<ON_Brep*> pieces;
	

	bool flag = RhinoBrepSplitRectangle(*split, curve, tol, pieces,context);

	int i, count = pieces.Count();
	if (count == 0 | count == 1)
	{
		RhinoApp().Print(L"BrepValid %d.\n", count);
		if (count == 1) {
			delete pieces[0];

		}

		return nothing;
	}

	const ON_3dVector mo(0, 1, 0);
	

	CRhinoObjectAttributes attrib = split_object->Attributes();
	attrib.m_uuid = ON_nil_uuid;

	const CRhinoObjectVisualAnalysisMode* vam_list = split_object->m_analysis_mode_list;

	for (i = 2; i < count; i++)
	{
		CRhinoBrepObject* brep_object = new CRhinoBrepObject(attrib);
		if (brep_object)
		{
			brep_object->SetBrep(pieces[i]);
			if (context.m_doc.AddObject(brep_object))
				RhinoCopyAnalysisModes(vam_list, brep_object);
			else
				delete brep_object;
		}
	}



	context.m_doc.DeleteObject(split_ref);
	context.m_doc.Redraw();





	return CRhinoCommand::success;


	//return success;
}

#pragma endregion

