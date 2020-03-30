//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/ShaderPrecache.h"
#include "../Graphics/ShaderVariation.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"

#include "../DebugNew.h"

namespace Urho3D
{

ShaderPrecache::ShaderPrecache(Context* context, const ea::string& fileName) :
    Object(context),
    fileName_(fileName),
    xmlFile_(context)
{
    if (GetSubsystem<FileSystem>()->FileExists(fileName))
    {
        // If file exists, read the already listed combinations
        File source(context_, fileName);
        xmlFile_.Load(source);

        XMLElement shader = xmlFile_.GetRoot().GetChild("shader");
        while (shader)
        {
            ea::string oldCombination = shader.GetAttribute("vs") + " " + shader.GetAttribute("vsdefines") + " " +
                                    shader.GetAttribute("ps") + " " + shader.GetAttribute("psdefines");
            usedCombinations_.insert(oldCombination);

            shader = shader.GetNext("shader");
        }
    }

    // If no file yet or loading failed, create the root element now
    if (!xmlFile_.GetRoot())
        xmlFile_.CreateRoot("shaders");

    URHO3D_LOGINFO("Begin dumping shaders to " + fileName_);
}

ShaderPrecache::~ShaderPrecache()
{
    URHO3D_LOGINFO("End dumping shaders");

    if (usedCombinations_.empty())
        return;

    File dest(context_, fileName_, FILE_WRITE);
    xmlFile_.Save(dest);
}

void ShaderPrecache::StoreShaders(ShaderVariation* vs, ShaderVariation* ps, ShaderVariation* hs, ShaderVariation* ds, ShaderVariation* gs)
{
    if (!vs || !ps)
        return;

    // Check for duplicate using pointers first (fast)
    //ShaderTuple shaderTuple = ea::make_tuple(vs, ps, hs, ds, gs);

    uint64_t shaderHash = MakeHash(vs);
    CombineHash(shaderHash, MakeHash(ps));
    if (gs || hs || ds)
    {
        CombineHash(shaderHash, MakeHash(gs));
        CombineHash(shaderHash, MakeHash(hs));
        CombineHash(shaderHash, MakeHash(ds));
    }

    if (usedPtrCombinations_.contains(shaderHash))
        return;
    usedPtrCombinations_.insert(shaderHash);

    ea::string vsName = vs->GetName();
    ea::string psName = ps->GetName();
    const ea::string& vsDefines = vs->GetDefines();
    const ea::string& psDefines = ps->GetDefines();

    // Check for duplicate using strings (needed for combinations loaded from existing file)
    ea::string newCombination = vsName + " " + vsDefines + " " + psName + " " + psDefines;

    if (gs)
        newCombination = newCombination + " " + gs->GetName() + " " + gs->GetDefines();
    if (hs)
        newCombination = newCombination + " " + hs->GetName() + " " + hs->GetDefines();
    if (ds)
        newCombination = newCombination + " " + ds->GetName() + " " + ds->GetDefines();

    if (usedCombinations_.contains(newCombination))
        return;
    usedCombinations_.insert(newCombination);

    XMLElement shaderElem = xmlFile_.GetRoot().CreateChild("shader");
    shaderElem.SetAttribute("vs", vsName);
    shaderElem.SetAttribute("vsdefines", vsDefines);
    shaderElem.SetAttribute("ps", psName);
    shaderElem.SetAttribute("psdefines", psDefines);

    if (gs)
    {
        shaderElem.SetAttribute("gs", gs->GetName());
        shaderElem.SetAttribute("gsdefines", gs->GetDefines());
    }

    if (hs)
    {
        shaderElem.SetAttribute("hs", hs->GetName());
        shaderElem.SetAttribute("hsdefines", hs->GetDefines());
    }

    if (ds)
    {
        shaderElem.SetAttribute("ds", ds->GetName());
        shaderElem.SetAttribute("dsdefines", ds->GetDefines());
    }
}

void ShaderPrecache::LoadShaders(Graphics* graphics, Deserializer& source)
{
    URHO3D_LOGDEBUG("Begin precaching shaders");

    XMLFile xmlFile(graphics->GetContext());
    xmlFile.Load(source);

    XMLElement shader = xmlFile.GetRoot().GetChild("shader");
    while (shader)
    {
        ea::string vsDefines = shader.GetAttribute("vsdefines");
        ea::string psDefines = shader.GetAttribute("psdefines");
        ea::string hsDefines = shader.GetAttribute("hsdefines");
        ea::string dsDefines = shader.GetAttribute("dsdefines");
        ea::string gsDefines = shader.GetAttribute("gsdefines");

        // Check for illegal variations on OpenGL ES and skip them
#ifdef GL_ES_VERSION_2_0
        if (
#ifndef __EMSCRIPTEN__
            vsDefines.contains("INSTANCED") ||
#endif
            (psDefines.contains("POINTLIGHT") && psDefines.contains("SHADOW")))
        {
            shader = shader.GetNext("shader");
            continue;
        }
#endif

        ShaderVariation* vs = graphics->GetShader(VS, shader.GetAttribute("vs"), vsDefines);
        ShaderVariation* ps = graphics->GetShader(PS, shader.GetAttribute("ps"), psDefines);
        ShaderVariation* hs = nullptr, *ds = nullptr, *gs = nullptr;

        if (shader.HasAttribute("hs")) hs = graphics->GetShader(HS, shader.GetAttribute("hs"), hsDefines);
        if (shader.HasAttribute("ds")) ds = graphics->GetShader(DS, shader.GetAttribute("ds"), hsDefines);
        if (shader.HasAttribute("gs")) gs = graphics->GetShader(GS, shader.GetAttribute("gs"), hsDefines);

        // Set the shaders active to actually compile them
        graphics->SetShaders(vs, ps, hs, ds, gs);

        shader = shader.GetNext("shader");
    }

    URHO3D_LOGDEBUG("End precaching shaders");
}

}
