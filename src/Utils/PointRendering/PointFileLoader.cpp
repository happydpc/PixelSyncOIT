/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2019, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <boost/algorithm/string/predicate.hpp>
#include <Utils/File/Logfile.hpp>
#include "import_uintah.h"
#include "import_cosmic_web.h"
#include "../MeshSerializer.hpp"
#include "../ImportanceCriteria.hpp"
#include "PointFileLoader.hpp"

// timestep.xml -> uintah
// .dat -> cosmic_web

void convertPointDataSetToBinmesh(
        const std::string &inputFilename,
        const std::string &binaryFilename) {
    sgl::Logfile::get()->writeInfo(std::string() + "Loading point data from \"" + inputFilename + "\"...");

    pl::ParticleModel particleModel;
    if (boost::ends_with(inputFilename, "timestep.xml")) {
        pl::import_uintah(pl::FileName(inputFilename), particleModel);
    } else if (boost::ends_with(inputFilename, ".dat")) {
        pl::import_cosmic_web(pl::FileName(inputFilename), particleModel);
    } else {
        sgl::Logfile::get()->writeError(
                std::string() + "Error: Unknown point data set file association for \""
                + inputFilename + "\"!");
        return;
    }

    auto positions = static_cast<pl::DataT<float>*>(particleModel["positions"].get());
    size_t numPoints = positions->size() / 3;
    glm::vec3 *positionValues = (glm::vec3*)positions->data.data();

    // Compute the bounding box.
    sgl::AABB3 aabb;
    for (size_t i = 0; i < numPoints; i++) {
        glm::vec3& position = positionValues[i];

        float tempZ = position.x;
        position.x = position.y;
        position.y = tempZ;
        aabb.combine(position);
    }

    // Find the maximum axis of the box.
    float largestAxis = 0.0f;
    for (size_t i = 0; i < 3; i++) {
        largestAxis = std::max(largestAxis, aabb.getExtent()[i]);
    }

    // Now normalize the data range.
    #pragma omp parallel for
    for (size_t i = 0; i < numPoints; i++) {
        glm::vec3 oldPoint = positionValues[i];
        positionValues[i] = (oldPoint - aabb.getCenter()) / largestAxis;
    }

    // Create a binary mesh from the data.
    BinaryMesh binaryMesh;
    binaryMesh.submeshes.resize(1);

    BinarySubMesh &binarySubmesh = binaryMesh.submeshes.at(0);
    binarySubmesh.vertexMode = sgl::VERTEX_MODE_POINTS;

    BinaryMeshAttribute positionAttribute;
    positionAttribute.name = "vertexPosition";
    positionAttribute.attributeFormat = sgl::ATTRIB_FLOAT;
    positionAttribute.numComponents = 3;
    positionAttribute.data.resize(numPoints * sizeof(glm::vec3));
    memcpy(&positionAttribute.data.front(), positions->data.data(), numPoints * sizeof(glm::vec3));
    binarySubmesh.attributes.push_back(positionAttribute);

    BinaryMeshAttribute vertexAttribute;
    vertexAttribute.name = "vertexAttribute0";
    vertexAttribute.attributeFormat = sgl::ATTRIB_UNSIGNED_SHORT;
    vertexAttribute.numComponents = 1;
    std::vector<uint16_t> vertexAttributeData(numPoints, 0u); // Just zero if no other attribute exists
    if (particleModel.find("velocities") != particleModel.end()) {
        // Cosmic web data set.
        // Compute velocity magnitudes as the vertex attribute.
        auto velocities = static_cast<pl::DataT<float>*>(particleModel["velocities"].get());
        assert(numPoints == velocities->size() / 3);
        glm::vec3 *velocityValues = (glm::vec3*)velocities->data.data();

        std::vector<float> velocityMagnitudes(numPoints);
        #pragma omp parallel for
        for (size_t i = 0; i < numPoints; i++) {
            velocityMagnitudes[i] = glm::length(velocityValues[i]);
        }
        packUnorm16Array(velocityMagnitudes, vertexAttributeData);
    } else if (particleModel.find("p.u") != particleModel.end() && particleModel.find("p.v") != particleModel.end()
            && particleModel.find("p.w") != particleModel.end()) {
        // Cosmic web data set.
        // Compute velocity magnitudes as the vertex attribute.
        auto us = static_cast<pl::DataT<double>*>(particleModel["p.u"].get());
        auto vs = static_cast<pl::DataT<double>*>(particleModel["p.v"].get());
        auto ws = static_cast<pl::DataT<double>*>(particleModel["p.w"].get());
        //assert(numPoints == rhos->size());
        double *uValues = (double*)us->data.data();
        double *vValues = (double*)vs->data.data();
        double *wValues = (double*)ws->data.data();

        std::vector<float> velocityMagnitudes(numPoints);
        #pragma omp parallel for
        for (size_t i = 0; i < numPoints; i++) {
            velocityMagnitudes[i] = glm::length(glm::vec3(uValues[i], vValues[i], wValues[i]));
        }
        packUnorm16Array(velocityMagnitudes, vertexAttributeData);
    }
    vertexAttribute.data.resize(vertexAttributeData.size() * sizeof(uint16_t));
    memcpy(&vertexAttribute.data.front(), &vertexAttributeData.front(), vertexAttributeData.size() * sizeof(uint16_t));
    binarySubmesh.attributes.push_back(vertexAttribute);

    // TODO: Test
    /*BinaryMeshAttribute positionAttribute;
    positionAttribute.name = "vertexPosition";
    positionAttribute.attributeFormat = sgl::ATTRIB_FLOAT;
    positionAttribute.numComponents = 3;
    positionAttribute.data.resize(1 * sizeof(glm::vec3), 0u);
    binarySubmesh.attributes.push_back(positionAttribute);

    BinaryMeshAttribute vertexAttribute;
    vertexAttribute.name = "vertexAttribute0";
    vertexAttribute.attributeFormat = sgl::ATTRIB_UNSIGNED_SHORT;
    vertexAttribute.numComponents = 1;
    vertexAttribute.data.resize(1 * sizeof(uint16_t), 0u);
    binarySubmesh.attributes.push_back(vertexAttribute);*/

    sgl::Logfile::get()->writeInfo(std::string() + "Writing binary mesh...");
    writeMesh3D(binaryFilename, binaryMesh);
    sgl::Logfile::get()->writeInfo(std::string() + "Finished writing binary mesh.");
}