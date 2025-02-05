#include "miniosgb.h"
#include <cstdio>
#include <memory>
#include <unordered_set>
#include <filesystem>

void ReadFile(const char* filename, bool dump);

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("  Usage:\n");
		printf("    Dump OSGB file :  testosgb <file>\n");
		printf("    Test OSGB files:  testosgb <dir>\n");
		printf("\n");
		return 0;
	}
	
	const std::filesystem::path path = argv[1];
	if (std::filesystem::is_directory(path)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
			if (entry.is_regular_file() && (entry.path().extension() == ".osgb")) {
				ReadFile(entry.path().string().c_str(), false);
			}
		}
	} else if (std::filesystem::is_regular_file(path)) {
		ReadFile(path.string().c_str(), true);
	} else {
		printf("FAILED: path not valid\n");
	}

	return 0;
}

void DumpObject(miniosgb::Object* obj, int level = 0);

void ReadFile(const char* filename, bool dump)
{
	printf_s("read %s ", filename);

	FILE* file = nullptr;
	fopen_s(&file, filename, "rb");
	if (file == nullptr) {
		printf("FAILED: can't open\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	const auto fileLen = ftell(file);
	const auto fileBuf = std::make_unique<unsigned char[]>((size_t)fileLen);
	fseek(file, 0, SEEK_SET);
	fread_s(fileBuf.get(), fileLen, 1, fileLen, file);
	fclose(file);

	std::string error;
	const auto data = miniosgb::Data::read(fileBuf.get(), fileLen, &error);
	if (data) {
		if (data->rootObject) {
			printf_s("OK\n");
			if (dump) {
				DumpObject(data->rootObject.get());
			}
		} else {
			printf_s("EMPTY\n");
		}
	} else {
		printf_s("FAILED: %s\n", error.c_str());
	}
}

std::unordered_set<miniosgb::Object*> dumpedObjects;
void DumpObject(miniosgb::Object* obj, int level) {
	const auto indent = std::string(size_t(level * 2), ' ');
	if (obj == nullptr) {
		//printf_s("%sNULL\n", indent.c_str());
		printf_s("NULL\n");
		return;
	}
	if (dumpedObjects.find(obj) != dumpedObjects.end()) {
		printf_s("%s(%d) {...}\n", obj->className(), obj->uniqueId);
		return;
	} else {
		dumpedObjects.insert(obj);
	}
	//printf_s("%sObject {\n", indent.c_str());
	printf_s("%s(%d) {", obj->className(), obj->uniqueId);
	if (const auto& node = dynamic_cast<miniosgb::Node*>(obj)) {
		printf_s("\n%s  <Node>\n", indent.c_str());
		printf_s("%s  StateSet= ", indent.c_str());
		DumpObject(node->stateSet.get(), level + 1);
		printf_s("%s", indent.c_str());
	}
	if (const auto& geode = dynamic_cast<miniosgb::Geode*>(obj)) {
		printf_s("\n%s  <Geode>\n", indent.c_str());
		printf_s("%s  Drawables= %zd [\n", indent.c_str(), geode->drawables.size());
		for (size_t i = 0, size = geode->drawables.size(); i < size; ++i) {
			const auto& drawable = geode->drawables[i];
			printf_s("%s    Drawable %zd: ", indent.c_str(), i);
			if (drawable) {
				DumpObject(drawable.get(), level + 2);
			}
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& primitiveSet = dynamic_cast<miniosgb::PrimitiveSet*>(obj)) {
		printf_s("\n%s  <PrimitiveSet>\n", indent.c_str());
		printf_s("%s  Mode= %d\n", indent.c_str(), primitiveSet->mode);
		printf_s("%s  IndexCount= %d\n", indent.c_str(), primitiveSet->indexCount);
		printf_s("%s  IndexData= %p\n", indent.c_str(), primitiveSet->indexData);
		printf_s("%s", indent.c_str());
	}
	if (const auto& geometry = dynamic_cast<miniosgb::Geometry*>(obj)) {
		printf_s("\n%s  <Geometry>\n", indent.c_str());
		printf_s("%s  Primitives= %zd [\n", indent.c_str(), geometry->primitives.size());
		for (size_t i = 0, size = geometry->primitives.size(); i < size; ++i) {
			const auto& prim = geometry->primitives[i];
			printf_s("%s    Primitive %zd: ", indent.c_str(), i);
			DumpObject(prim.get(), level + 2);
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s  VertexData: ", indent.c_str());
		DumpObject(geometry->vertexData.get(), level + 1);
		printf_s("%s  TexCoordDataList: %zd [\n", indent.c_str(), geometry->texCoordDataList.size());
		for (size_t i = 0, size = geometry->texCoordDataList.size(); i < size; ++i) {
			const auto& texCoordData = geometry->texCoordDataList[i];
			printf_s("%s    TexCoordData %zd: ", indent.c_str(), i);
			DumpObject(texCoordData.get(), level + 2);
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& group = dynamic_cast<miniosgb::Group*>(obj)) {
		printf_s("\n%s  <Group>\n", indent.c_str());
		printf_s("%s  Children= %zd [\n", indent.c_str(), group->children.size());
		for (size_t i = 0, size = group->children.size(); i < size; ++i) {
			const auto& child = group->children[i];
			printf_s("%s    Child %zd: ", indent.c_str(), i);
			DumpObject(child.get(), level + 2);
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& lod = dynamic_cast<miniosgb::LOD*>(obj)) {
		printf_s("\n%s  <LOD>\n", indent.c_str());
		printf_s("%s  CenterMode= %d\n", indent.c_str(), lod->centerMode);
		printf_s("%s  UserDefinedCenter= (%f, %f, %f)\n", indent.c_str(), lod->userDefinedCenter.x, lod->userDefinedCenter.y, lod->userDefinedCenter.z);
		printf_s("%s  UserDefinedRadius= %f\n", indent.c_str(), lod->userDefinedRadius);
		printf_s("%s  RangeList= %zd [\n", indent.c_str(), lod->rangeList.size());
		for (size_t i = 0, size = lod->rangeList.size(); i < size; ++i) {
			const auto& range = lod->rangeList[i];
			printf_s("%s    RangeList %zd= { Min=%g, Max=%g }\n", indent.c_str(), i, range.min, range.max);
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& plod = dynamic_cast<miniosgb::PagedLOD*>(obj)) {
		printf_s("\n%s  <PagedLOD>\n", indent.c_str());
		printf_s("%s  RangeDataList= %zd [\n", indent.c_str(), plod->rangeDataList.size());
		for (size_t i = 0, size = plod->rangeDataList.size(); i < size; ++i) {
			printf_s("%s    RangeData %zd:\n", indent.c_str(), i);
			const auto& rangeData = plod->rangeDataList[i];
			printf_s("%s      Filename= %s\n", indent.c_str(), rangeData.filename.c_str());
			printf_s("%s      PriorityOffset= %f\n", indent.c_str(), rangeData.priorityOffset);
			printf_s("%s      PriorityScale= %f\n", indent.c_str(), rangeData.priorityScale);
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& arr = dynamic_cast<miniosgb::Array*>(obj)) {
		printf_s("\n%s  <Array>\n", indent.c_str());
		printf_s("%s  ArrayType= %d\n", indent.c_str(), arr->arrayType);
		printf_s("%s  ElementSize= %d\n", indent.c_str(), arr->elementSize);
		printf_s("%s  ElementCount= %d\n", indent.c_str(), arr->elementCount);
		printf_s("%s  Binding= %d\n", indent.c_str(), arr->binding);
		printf_s("%s  Normalize= %d\n", indent.c_str(), arr->normalize);
		printf_s("%s", indent.c_str());
	}
	if (const auto& stateSet = dynamic_cast<miniosgb::StateSet*>(obj)) {
		printf_s("\n%s  <StateSet>\n", indent.c_str());
		printf_s("%s  RenderingHint= %d\n", indent.c_str(), stateSet->renderingHint);
		printf_s("%s  Attributes= %zd [\n", indent.c_str(), stateSet->attributes.size());
		for (size_t i = 0, size = stateSet->attributes.size(); i < size; ++i) {
			const auto& p = stateSet->attributes[i];
			printf_s("%s    {\n", indent.c_str());
			printf_s("%s      Attribute %zd: ", indent.c_str(), i);
			DumpObject(p.first.get(), level + 3);
			printf_s("%s      Value= %d\n", indent.c_str(), p.second);
			printf_s("%s    }\n", indent.c_str());
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s  TextureAttributesList= %zd [\n", indent.c_str(), stateSet->textureAttributesList.size());
		for (size_t i = 0, size = stateSet->textureAttributesList.size(); i < size; ++i) {
			const auto& texutreAttributes = stateSet->textureAttributesList[i];
			printf_s("%s    TextureAttributes %zd= %zd: [\n", indent.c_str(), i, texutreAttributes.size());
			for (size_t j = 0, size_ = texutreAttributes.size(); j < size_; ++j) {
				const auto& p = texutreAttributes[j];
				printf_s("%s      {\n", indent.c_str());
				printf_s("%s        TextureAttribute %zd: ", indent.c_str(), j);
				DumpObject(p.first.get(), level + 4);
				printf_s("%s        Value= %d\n", indent.c_str(), p.second);
				printf_s("%s      }\n", indent.c_str());
			}
			printf_s("%s    ]\n", indent.c_str());
		}
		printf_s("%s  ]\n", indent.c_str());
		printf_s("%s", indent.c_str());
	}
	if (const auto& material = dynamic_cast<miniosgb::Material*>(obj)) {
		printf_s("\n%s  <Material>\n", indent.c_str());
		printf_s("%s    Ambient:\n", indent.c_str());
		printf_s("%s      FrontAndBack= %d\n", indent.c_str(), material->ambient.frontAndBack);
		printf_s("%s      Front= (%f, %f, %f, %f)\n", indent.c_str(), material->ambient.front.x, material->ambient.front.y, material->ambient.front.z, material->ambient.front.w);
		printf_s("%s      Back= (%f, %f, %f, %f)\n", indent.c_str(), material->ambient.back.x, material->ambient.back.y, material->ambient.back.z, material->ambient.back.w);
		printf_s("%s    Diffuse:\n", indent.c_str());
		printf_s("%s      FrontAndBack= %d\n", indent.c_str(), material->diffuse.frontAndBack);
		printf_s("%s      Front= (%f, %f, %f, %f)\n", indent.c_str(), material->diffuse.front.x, material->diffuse.front.y, material->diffuse.front.z, material->diffuse.front.w);
		printf_s("%s      Back= (%f, %f, %f, %f)\n", indent.c_str(), material->diffuse.back.x, material->diffuse.back.y, material->diffuse.back.z, material->diffuse.back.w);
		printf_s("%s    Specular:\n", indent.c_str());
		printf_s("%s      FrontAndBack= %d\n", indent.c_str(), material->specular.frontAndBack);
		printf_s("%s      Front= (%f, %f, %f, %f)\n", indent.c_str(), material->specular.front.x, material->specular.front.y, material->specular.front.z, material->specular.front.w);
		printf_s("%s      Back= (%f, %f, %f, %f)\n", indent.c_str(), material->specular.back.x, material->specular.back.y, material->specular.back.z, material->specular.back.w);
		printf_s("%s    Emission:\n", indent.c_str());
		printf_s("%s      FrontAndBack= %d\n", indent.c_str(), material->emission.frontAndBack);
		printf_s("%s      Front= (%f, %f, %f, %f)\n", indent.c_str(), material->emission.front.x, material->emission.front.y, material->emission.front.z, material->emission.front.w);
		printf_s("%s      Back= (%f, %f, %f, %f)\n", indent.c_str(), material->emission.back.x, material->emission.back.y, material->emission.back.z, material->emission.back.w);
		printf_s("%s    Shininess:\n", indent.c_str());
		printf_s("%s      FrontAndBack= %d\n", indent.c_str(), material->shininess.frontAndBack);
		printf_s("%s      Front= %f\n", indent.c_str(), material->shininess.front);
		printf_s("%s      Back= %f\n", indent.c_str(), material->shininess.back);
		printf_s("%s", indent.c_str());
	}
	if (const auto& texture = dynamic_cast<miniosgb::Texture*>(obj)) {
		printf_s("\n%s  <Texture>\n", indent.c_str());
		printf_s("%s  WrapS= 0x%X\n", indent.c_str(), texture->wrapS);
		printf_s("%s  WrapT= 0x%X\n", indent.c_str(), texture->wrapT);
		printf_s("%s  WrapR= 0x%X\n", indent.c_str(), texture->wrapR);
		printf_s("%s", indent.c_str());
	}
	if (const auto& texture2D = dynamic_cast<miniosgb::Texture2D*>(obj)) {
		printf_s("\n%s  <Texture2D>\n", indent.c_str());
		printf_s("%s  Image: ", indent.c_str());
		DumpObject(texture2D->image.get(), level + 1);
		printf_s("%s", indent.c_str());
	}
	if (const auto& image = dynamic_cast<miniosgb::Image*>(obj)) {
		printf_s("\n%s  <Image>\n", indent.c_str());
		printf_s("%s  Data= %p\n", indent.c_str(), image->data);
		printf_s("%s  DataLength= %d\n", indent.c_str(), image->dataLength);
		printf_s("%s", indent.c_str());
	}
	printf_s("}\n");
}
