#include "LimbSource.hpp"

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "numerics/Eigen.hpp"
#include <array>

vtkStandardNewMacro(LimbSource)

LimbSource::LimbSource()
{
    this->SetNumberOfInputPorts(0);
}

void LimbSource::SetLimbData(const LimbProperties& limb)
{
    this->limb = limb;
    this->Modified();
}

int LimbSource::RequestData(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
    // Generate points

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetDataType(VTK_DOUBLE);
    //newPoints->Allocate(...);      // Todo: Preallocate

    size_t n_sections = limb.s.size();
    for(size_t i = 0; i < n_sections; ++i)   // Iterate over sections
    {
        // Curve point and normals
        Vector<3> p{limb.x[i], limb.y[i], 0.0};
        Vector<3> nw{0.0, 0.0, 0.5*abs(limb.w[i])};
        Vector<3> nh{-abs(limb.h[i])*sin(limb.phi[i]), abs(limb.h[i])*cos(limb.phi[i]), 0.0};

        // Cross section vertices
        Vector<3> p0 = p + nw;
        Vector<3> p1 = p + nw + nh;
        Vector<3> p2 = p - nw + nh;
        Vector<3> p3 = p - nw;

        points->InsertNextPoint(p0.data());
        points->InsertNextPoint(p1.data());
        points->InsertNextPoint(p2.data());
        points->InsertNextPoint(p3.data());
    }

    // Generate indices

    auto polys = vtkSmartPointer<vtkCellArray>::New();
    //newPolys->Allocate(...);        // Todo: Preallocate

    size_t n_segments = (n_sections > 0) ? n_sections-1 : 0;
    for(size_t i = 0; i < n_segments; ++i)
    {
        vtkIdType i0 = 4*i;        // Beginning index of first section
        vtkIdType i1 = i0 + 4;     // Beginning index of second section

        if(i == 0)
        {
            vtkIdType pts_front[] = {i0 + 0, i0 + 1, i0 + 2, i0 + 3};
            polys->InsertNextCell(4, pts_front);
        }
        else if(i == n_segments - 1)
        {
            vtkIdType pts_back[] = {i1 + 0, i1 + 3, i1 + 2, i1 + 1};
            polys->InsertNextCell(4, pts_back);
        }

        vtkIdType pts_top[] = {i0 + 0, i0 + 3, i1 + 3, i1 + 0};
        vtkIdType pts_bottom[] = {i0 + 1, i1 + 1, i1 + 2, i0 + 2};
        vtkIdType pts_left[] = {i0 + 0, i1 + 0, i1 + 1, i0 + 1};
        vtkIdType pts_right[] = {i0 + 2, i1 + 2, i1 + 3, i0 + 3};

        polys->InsertNextCell(4, pts_top);
        polys->InsertNextCell(4, pts_bottom);
        polys->InsertNextCell(4, pts_left);
        polys->InsertNextCell(4, pts_right);
    }

    // Set output stuff

    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkPolyData* output = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    output->SetPoints(points);
    output->SetPolys(polys);

    return 1;
}