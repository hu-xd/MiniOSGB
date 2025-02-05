#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace miniosgb
{
	struct Vec2f { float x = 0; float y = 0; };
	struct Vec3f { float x = 0; float y = 0; float z = 0; };
	struct Vec4f { float x = 0; float y = 0; float z = 0; float w = 0; };
	struct Vec3d { double x = 0; double y = 0; double z = 0; };

	struct Object {
		unsigned int uniqueId;
		virtual const char* className() const = 0;
		virtual ~Object() {}
	};

	struct Array : Object {
		enum class ArrayType { Unknown = 0, Vec2f = 27, Vec3f = 28, Vec4f = 29 };
		const ArrayType arrayType;
		const unsigned int elementSize;
		Array(ArrayType arrayType_, unsigned int elementSize_) : arrayType(arrayType_), elementSize(elementSize_) {}

		unsigned int elementCount = 0;
		const unsigned char* elementData = nullptr;
		enum class Binding { Undefined = -1, Off = 0, Overall = 1, PerPrimitiveSet = 2, PerVertex = 4 };
		Binding binding = Binding::Off;
		bool normalize = false;

		bool virtual readFloats(unsigned int index, float* dest, unsigned int count) = 0;
	};

	struct Vec2Array : Array {
		Vec2Array() : Array(ArrayType::Vec2f, sizeof(Vec2f)) {}
		const char* className() const override { return "Vec2Array"; }
		bool virtual readFloats(unsigned int index, float* dest, unsigned int count) override {
			if ((index >= elementCount) || (count > 2)) {
				return false;
			}
			memcpy(dest, (Vec2f*)elementData + index, sizeof(float) * count);
			return true;
		}
	};

	struct Vec3Array : Array {
		Vec3Array() : Array(ArrayType::Vec3f, sizeof(Vec3f)) {}
		const char* className() const override { return "Vec3Array"; }
		bool virtual readFloats(unsigned int index, float* dest, unsigned int count) override {
			if ((index >= elementCount) || (count > 3)) {
				return false;
			}
			memcpy(dest, (Vec3f*)elementData + index, sizeof(float) * count);
			return true;
		}
	};

	struct Vec4Array : Array {
		Vec4Array() : Array(ArrayType::Vec4f, sizeof(Vec4f)) {}
		const char* className() const override { return "Vec4Array"; }
		bool virtual readFloats(unsigned int index, float* dest, unsigned int count) override {
			if ((index >= elementCount) || (count > 4)) {
				return false;
			}
			memcpy(dest, (Vec4f*)elementData + index, sizeof(float) * count);
			return true;
		}
	};

	struct BufferData : Object {};

	// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/include/osg/StateAttribute
	struct StateAttribute : Object {
		typedef unsigned int GLMode;
		typedef unsigned int GLModeValue;
		typedef unsigned int OverrideValue;
		enum class Values { Off = 0x0, On = 0x1, Override = 0x2, Protected = 0x4, Inherit = 0x8 };
	};

	// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/include/osg/StateSet
	struct StateSet : Object {
		const char* className() const override { return "StateSet"; }
		typedef std::vector<std::pair<StateAttribute::GLMode, StateAttribute::GLModeValue>> ModeList;
		ModeList modes;

		typedef std::vector<std::pair<std::shared_ptr<StateAttribute>, StateAttribute::OverrideValue>> AttributeList;
		AttributeList attributes;

		typedef std::vector<ModeList> TextureModeList;
		TextureModeList textureModesList;

		typedef std::vector<AttributeList> TextureAttributeList;
		TextureAttributeList textureAttributesList;

		enum class RenderingHint { DefaultBin = 0, OpaqueBin = 1, TransparentBin = 2 };
		RenderingHint renderingHint = RenderingHint::DefaultBin;
	};

	struct Node : Object {
		std::shared_ptr<StateSet> stateSet;
	};

	struct Drawable : Node {};

	struct PrimitiveSet : BufferData {
		const char* className() const override { return "PrimitiveSet"; }
		unsigned int mode = 0;
		// unconfirmed
		const unsigned char* indexData = nullptr;
		unsigned int indexCount = 0;
	};

	struct DrawElementsUInt : PrimitiveSet {
		const char* className() const override { return "DrawElementsUInt"; }
	};

	struct Geometry : Drawable {
		const char* className() const override { return "Geometry"; }
		std::vector<std::shared_ptr<PrimitiveSet>> primitives;
		std::shared_ptr<Array> vertexData;
		std::shared_ptr<Array> normalData;
		std::shared_ptr<Array> colorData;
		std::shared_ptr<Array> secondaryColorData;
		std::shared_ptr<Array> fogCoordData;
		std::vector<std::shared_ptr<Array>> texCoordDataList;
	};

	struct Geode : Node {
		const char* className() const override { return "Geode"; }
		std::vector<std::shared_ptr<Drawable>> drawables;
	};

	struct Group : Node {
		const char* className() const override { return "Group"; }
		std::vector<std::shared_ptr<Node>> children;
	};

	struct LOD : Group {
		int centerMode = 0;
		Vec3d userDefinedCenter;
		double userDefinedRadius = 0;

		struct Range { float min; float max; };
		std::vector<Range> rangeList;
	};

	struct PagedLOD : LOD {
		const char* className() const override { return "PagedLOD"; }
		struct RangeData {
			std::string filename;
			float priorityOffset = 0;
			float priorityScale = 0;
		};
		std::vector<RangeData> rangeDataList;
	};

	struct Material : StateAttribute {
		const char* className() const override { return "Material"; }
		template <typename T> struct Property {
			bool frontAndBack;
			T front;
			T back;
		};
		Property<Vec4f> ambient;
		Property<Vec4f> diffuse;
		Property<Vec4f> specular;
		Property<Vec4f> emission;
		Property<float> shininess;
	};

	struct Texture : StateAttribute {
		enum class WrapMode {
			Clamp = 0x2900, // GL_CLAMP,
			ClampToEdge = 0x812F, // GL_CLAMP_TO_EDGE,
			ClampToBorder = 0x812D, // GL_CLAMP_TO_BORDER_ARB,
			Repeat = 0x2901, // GL_REPEAT,
			Mirror = 0x8370, // GL_MIRRORED_REPEAT_IBM
		};
		WrapMode wrapS = WrapMode::ClampToEdge;
		WrapMode wrapT = WrapMode::ClampToEdge;
		WrapMode wrapR = WrapMode::ClampToEdge;
	};

	struct Image : BufferData {
		const char* className() const override { return "Image"; }
		const unsigned char* data = nullptr;
		unsigned int dataLength = 0;
	};

	struct Texture2D : Texture {
		const char* className() const override { return "Texture2D"; }
		std::shared_ptr<Image> image;
	};

	struct UserDataContainer : Object {};
	struct DefaultUserDataContainer : UserDataContainer {
		const char* className() const override { return "DefaultUserDataContainer"; }
	};

	namespace details {
		struct Reader {
			struct Error : std::runtime_error {
				const size_t offset;
				Error(size_t offset_, const std::string& message) : std::runtime_error(message), offset(offset_) {}
			};

			Reader(const unsigned char* buffer, size_t length)
				: _buffer(buffer), _length(length) {
			}

			const unsigned char* _buffer;
			const size_t _length;
			size_t _pos = 0;

			bool ended() const {
				return (_pos == _length);
			}

			unsigned int _version = 0;
			bool _useBinaryBrackets = false;

			template<typename T> T read()
			{
				if ((_pos + sizeof(T) > _length)) {
					throw Error(_pos, "read beyond data length");
				}
				const auto ptr = (T*)(_buffer + _pos);
				_pos += sizeof(T);
				return *ptr;
			}

			template<typename T> void read(T* value) {
				if ((_pos + sizeof(T) > _length)) {
					throw Error(_pos, "read beyond data length");
				}
				*value = *(T*)(_buffer + _pos);
				_pos += sizeof(T);
			}

			template<typename T> void read(T* value, size_t count) {
				if ((_pos + sizeof(T) * count > _length)) {
					throw Error(_pos, "read beyond data length");
				}
				for (size_t i = 0; i < count; ++i) {
					*(value + i) = *(T*)(_buffer + _pos);
					_pos += sizeof(T);
				}
			}

			template<> bool read<bool>() {
				if ((_pos + sizeof(bool) > _length)) {
					throw Error(_pos, "read beyond data length");
				}
				const auto value = *(bool*)(_buffer + _pos);
				if ((value != true) && (value != false)) {
					throw Error(_pos, "invalid bool value");
				}
				_pos += sizeof(bool);
				return value;
			}

			template<>
			std::string read<std::string>() {
				// readString https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgPlugins/osg/BinaryStreamOperator.h
				const auto size = read<int>();
				if (size < 0) {
					throw Error(_pos, "invalid string length");
				}
				std::string s(size, '\0');
				read(const_cast<char*>(s.data()), size);
				return s;
			}

			std::shared_ptr<Object> readObjectIfTrue() {
				// ObjectSerializer https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/include/osgDB/Serializer
				if (read<bool>()) {
					return readObject();
				} else {
					return nullptr;
				}
			}

			template<typename T> std::shared_ptr<T> readObjectData();

			template<> std::shared_ptr<PagedLOD> readObjectData<PagedLOD>() {
				auto obj = std::make_shared<PagedLOD>();
				readObjectFields<Object>(*obj);
				readObjectFields<Node>(*obj);
				readObjectFields<LOD>(*obj);
				readObjectFields<PagedLOD>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Group> readObjectData<Group>() {
				auto obj = std::make_shared<Group>();
				readObjectFields<Object>(*obj);
				readObjectFields<Node>(*obj);
				readObjectFields<Group>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Geode> readObjectData<Geode>() {
				auto obj = std::make_shared<Geode>();
				readObjectFields<Object>(*obj);
				readObjectFields<Node>(*obj);
				readObjectFields<Geode>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Geometry> readObjectData<Geometry>() {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/Geometry.cpp
				auto obj = std::make_shared<Geometry>();
				readObjectFields<Object>(*obj);
				if (_version >= 154) {
					readObjectFields<Node>(*obj);
				}
				readObjectFields<Drawable>(*obj);
				readObjectFields<Geometry>(*obj);
				return obj;
			}

			template<> std::shared_ptr<DrawElementsUInt> readObjectData<DrawElementsUInt>() {
				auto obj = std::make_shared<DrawElementsUInt>();
				readObjectFields<Object>(*obj);
				readObjectFields<PrimitiveSet>(*obj);
				readObjectFields<DrawElementsUInt>(*obj);
				return obj;
			}

			template<> std::shared_ptr<StateSet> readObjectData<StateSet>() {
				auto obj = std::make_shared<StateSet>();
				readObjectFields<Object>(*obj);
				readObjectFields<StateSet>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Material> readObjectData<Material>() {
				auto obj = std::make_shared<Material>();
				readObjectFields<Object>(*obj);
				readObjectFields<StateAttribute>(*obj);
				readObjectFields<Material>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Texture2D> readObjectData<Texture2D>() {
				auto obj = std::make_shared<Texture2D>();
				readObjectFields<Object>(*obj);
				readObjectFields<StateAttribute>(*obj);
				readObjectFields<Texture>(*obj);
				readObjectFields<Texture2D>(*obj);
				return obj;
			}

			template<> std::shared_ptr<DefaultUserDataContainer> readObjectData<DefaultUserDataContainer>() {
				auto obj = std::make_shared<DefaultUserDataContainer>();
				readObjectFields<Object>(*obj);
				readObjectFields<DefaultUserDataContainer>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Vec3Array> readObjectData<Vec3Array>() {
				auto obj = std::make_shared<Vec3Array>();
				readObjectFields<Object>(*obj);
				readObjectFields<Array>(*obj);
				readObjectFields<Vec3Array>(*obj);
				return obj;
			}

			template<> std::shared_ptr<Vec2Array> readObjectData<Vec2Array>() {
				auto obj = std::make_shared<Vec2Array>();
				readObjectFields<Object>(*obj);
				readObjectFields<Array>(*obj);
				readObjectFields<Vec2Array>(*obj);
				return obj;
			}

			template<typename T> void readObjectFields(T& obj);

			template<> void readObjectFields<Object>(Object& obj) {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/Object.cpp
				const auto name = read<std::string>();
				read<unsigned int>(); // dataVariance
				if (_version < 77) { // UserData
					readObject();
				} else { // UserDataContainer
					readObjectIfTrue();
				}
			}

			template<> void readObjectFields<Node>(Node& obj) {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/Node.cpp
				if (read<bool>()) { // InitialBound
					ReadBeginBracket();
					const auto centerX = read<double>();
					const auto centerY = read<double>();
					const auto centerZ = read<double>();
					const auto radius = read<float>();
					ReadEndBracket();
				}
				readObjectIfTrue(); // computeBoundCallback
				readObjectIfTrue(); // updateCallback
				readObjectIfTrue(); // eventCallback
				readObjectIfTrue(); // cullCallback
				read<bool>(); // cullingActive
				read<unsigned int>(); // nodeMask
				if ((_version < 77) && read<bool>()) { // descriptions
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						read<std::string>();
					}
					ReadEndBracket();
				}
				obj.stateSet = std::dynamic_pointer_cast<StateSet>(readObjectIfTrue());
			}

			template<> void readObjectFields<Group>(Group& obj) {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/Group.cpp
				if (read<bool>()) { // Children
					const auto size = read<unsigned int>();
					obj.children.resize(size);
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						obj.children[i] = std::dynamic_pointer_cast<Node>(readObject());
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<LOD>(LOD& obj) {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/LOD.cpp
				obj.centerMode = read<int>();
				if (read<bool>()) { // userDefinedCenter, radius
					obj.userDefinedCenter = read<Vec3d>();
					obj.userDefinedRadius = read<double>();
				}
				const auto rangeMode = read<unsigned int>();
				if (read<bool>()) { // RangeList
					const auto size = read<unsigned int>();
					obj.rangeList.resize(size);
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						const auto min = read<float>();
						const auto max = read<float>();
						obj.rangeList[i] = { min, max };
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<PagedLOD>(PagedLOD& obj) {
				// https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgWrappers/serializers/osg/PagedLOD.cpp
				if (read<bool>()) {
					const auto hasDatabasePath = read<bool>();
					if (hasDatabasePath) {
						const auto databasePath = read<std::string>();
					}
				}
				if (_version < 70) {
					read<unsigned int>(); // frameNumberOfLastTraversal
				}
				read<unsigned int>(); // numChildrenThatCannotBeExpired
				read<bool>(); // disableExternalChildrenPaging
				if (read<bool>()) { // RangedDataList
					const auto fsize = read<unsigned int>();
					obj.rangeDataList.resize(fsize);
					ReadBeginBracket();
					for (unsigned int i = 0; i < fsize; ++i) {
						obj.rangeDataList[i].filename = read<std::string>();
					}
					ReadEndBracket();
					const auto psize = read<unsigned int>();
					if (psize > fsize) {
						obj.rangeDataList.resize(psize);
					}
					ReadBeginBracket();
					for (unsigned int i = 0; i < psize; ++i) {
						obj.rangeDataList[i].priorityOffset = read<float>();
						obj.rangeDataList[i].priorityScale = read<float>();
					}
					ReadEndBracket();
				}
				if (read<bool>()) { // Childeren
					const auto size = read<unsigned int>();
					obj.children.resize(size);
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						obj.children[i] = std::dynamic_pointer_cast<Node>(readObject());
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<Geode>(Geode& obj) {
				if (read<bool>()) { // Drawables
					const auto size = read<unsigned int>();
					obj.drawables.resize(size);
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						obj.drawables[i] = std::dynamic_pointer_cast<Drawable>(readObject());
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<Drawable>(Drawable& obj) {
				obj.stateSet = std::dynamic_pointer_cast<StateSet>(readObjectIfTrue());
				if (read<bool>()) { // InitialBound
					double dummy[6];
					read(dummy, 6);
				}
				readObjectIfTrue(); // computeBoundingBoxCallback
				readObjectIfTrue(); // shape
				read<bool>(); // supportsDisplayList
				read<bool>(); // useDisplayList
				read<bool>(); // useVertexBufferObjects
				readObjectIfTrue(); // updateCallback
				readObjectIfTrue(); // eventCallback
				readObjectIfTrue(); // cullCallback
				readObjectIfTrue(); // drawCallback
				//TODO: MORE
			}

			template<> void readObjectFields<PrimitiveSet>(PrimitiveSet& obj) {
				read<int>(); // NumInstances;
				obj.mode = read<unsigned int>();
				obj.indexCount = read<unsigned int>();
				obj.indexData = _buffer + _pos;
			}

			template<> void readObjectFields<DrawElementsUInt>(DrawElementsUInt& obj) {
				_pos += obj.indexCount * sizeof(unsigned int);
			}

			template<> void readObjectFields<Geometry>(Geometry& obj) {
				{ // PrimitiveSet
					const auto size = read<unsigned int>();
					if (_version < 112) {
						ReadBeginBracket();
						obj.primitives.resize(size);
						for (unsigned int p = 0; p < size; ++p) {
							const auto prim = std::make_shared<PrimitiveSet>();
							read<unsigned int>(); // NumInstances
							prim->mode = read<unsigned int>();

							// unconfirmed
							const auto indexCount = read<unsigned int>();
							prim->indexCount = indexCount;
							if (indexCount > 0) {
								prim->indexData = _buffer + _pos;
								_pos += sizeof(unsigned int) * indexCount;
							}
							obj.primitives[p] = prim;
						}
						ReadEndBracket();
					} else {
						obj.primitives.resize(size);
						for (unsigned int p = 0; p < size; ++p) {
							obj.primitives[p] = std::dynamic_pointer_cast<PrimitiveSet>(readObject());
						}
					}
				}
				if (_version < 112) {
					if (read<bool>()) {
						ReadBeginBracket();
						obj.vertexData = ReadArray();
						ReadEndBracket();
					}
					if (read<bool>()) {
						ReadBeginBracket();
						ReadArray();
						ReadEndBracket();
					}
					if (read<bool>()) {
						ReadBeginBracket();
						ReadArray();
						ReadEndBracket();
					}
					if (read<bool>()) {
						ReadBeginBracket();
						ReadArray();
						ReadEndBracket();
					}
					if (read<bool>()) {
						ReadBeginBracket();
						ReadArray();
						ReadEndBracket();
					}
					if (read<bool>()) {
						const auto size = read<unsigned int>();
						obj.texCoordDataList.resize(size);
						ReadBeginBracket();
						for (unsigned int i = 0; i < size; ++i) {
							ReadBeginBracket();
							obj.texCoordDataList[i] = ReadArray();
							ReadEndBracket();
						}
						ReadEndBracket();
					}
					if (read<bool>()) { // VertexAttribData
						const auto size = read<unsigned int>();
						obj.texCoordDataList.resize(size);
						ReadBeginBracket();
						for (unsigned int i = 0; i < size; ++i) {
							ReadBeginBracket();
							ReadArray();
							ReadEndBracket();
						}
						ReadEndBracket();
					}
					read<bool>(); // FastPathHint
				} else {
					obj.vertexData = std::dynamic_pointer_cast<Array>(readObjectIfTrue());
					obj.normalData = std::dynamic_pointer_cast<Array>(readObjectIfTrue());
					obj.colorData = std::dynamic_pointer_cast<Array>(readObjectIfTrue());
					obj.secondaryColorData = std::dynamic_pointer_cast<Array>(readObjectIfTrue());
					obj.fogCoordData = std::dynamic_pointer_cast<Array>(readObjectIfTrue());
					{
						const auto size = read<unsigned int>();
						obj.texCoordDataList.resize(size);
						for (unsigned int i = 0; i < size; ++i) {
							obj.texCoordDataList[i] = std::dynamic_pointer_cast<Array>(readObject());
						}
					}
					{ // VertexAttribData
						const auto size = read<unsigned int>();
						for (unsigned int i = 0; i < size; ++i) {
							const auto vertexAttribData = std::dynamic_pointer_cast<Array>(readObject());
						}
					}
				}
			}

			template<> void readObjectFields<StateSet>(StateSet& obj) {
				if (read<bool>()) {
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						const auto mode = read<unsigned int>();
						const auto value = read<unsigned int>();
						obj.modes.emplace_back(mode, value);
					}
					ReadEndBracket();
				}
				if (read<bool>()) {
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						const auto attribute = std::dynamic_pointer_cast<StateAttribute>(readObject());
						const auto value = read<unsigned int>();
						if (attribute) {
							obj.attributes.emplace_back(attribute, value);
						}
					}
					ReadEndBracket();
				}
				if (read<bool>()) {
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						StateSet::ModeList modes;
						const auto size_ = read<unsigned int>();
						ReadBeginBracket();
						for (unsigned int j = 0; j < size_; ++j) {
							const auto mode = read<unsigned int>();
							const auto value = read<unsigned int>();
							modes.emplace_back(mode, value);
						}
						ReadEndBracket();
						obj.textureModesList.emplace_back(std::move(modes));
					}
					ReadEndBracket();
				}
				if (read<bool>()) {
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						StateSet::AttributeList attributes;
						const auto size_ = read<unsigned int>();
						ReadBeginBracket();
						for (unsigned int j = 0; j < size_; ++j) {
							auto attribute = std::dynamic_pointer_cast<StateAttribute>(readObject());
							const auto value = read<unsigned int>();
							if (attribute) {
								attributes.emplace_back(attribute, value);
							}
						}
						ReadEndBracket();
						obj.textureAttributesList.emplace_back(std::move(attributes));
					}
					ReadEndBracket();
				}
				if (read<bool>()) { // UniformList
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						readObject();
						const auto value = read<unsigned int>();
					}
					ReadEndBracket();
				}
				obj.renderingHint = read<StateSet::RenderingHint>();
				const auto renderBinMode = read<unsigned int>();
				const auto binNumber = read<unsigned int>();
				const auto binName = read<std::string>();
				const auto nestRenderBins = read<bool>();
				readObjectIfTrue();
				readObjectIfTrue();
				if ((_version >= 151) && read<bool>()) {
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						read<std::string>();
						read<std::string>();
						read<int>();
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<StateAttribute>(StateAttribute& obj) {
				readObjectIfTrue();
				readObjectIfTrue();
			}

			template<> void readObjectFields<Material>(Material& obj) {
				const auto colorMode = read<unsigned int>();
				if (read<bool>()) {
					obj.ambient.frontAndBack = read<bool>();
					obj.ambient.front = read<Vec4f>();
					obj.ambient.back = read<Vec4f>();
				}
				if (read<bool>()) {
					obj.diffuse.frontAndBack = read<bool>();
					obj.diffuse.front = read<Vec4f>();
					obj.diffuse.back = read<Vec4f>();
				}
				if (read<bool>()) {
					obj.specular.frontAndBack = read<bool>();
					obj.specular.front = read<Vec4f>();
					obj.specular.back = read<Vec4f>();
				}
				if (read<bool>()) {
					obj.emission.frontAndBack = read<bool>();
					obj.emission.front = read<Vec4f>();
					obj.emission.back = read<Vec4f>();
				}
				if (read<bool>()) {
					obj.shininess.frontAndBack = read<bool>();
					obj.shininess.front = read<float>();
					obj.shininess.back = read<float>();
				}
			}

			template<> void readObjectFields<Texture>(Texture& obj) {
				if (read<bool>()) {
					obj.wrapS = read<Texture::WrapMode>();
				}
				if (read<bool>()) {
					obj.wrapT = read<Texture::WrapMode>();
				}
				if (read<bool>()) {
					obj.wrapR = read<Texture::WrapMode>();
				}
				if (read<bool>()) {
					const auto minFilter = read<unsigned int>();
				}
				if (read<bool>()) {
					const auto maxFilter = read<unsigned int>();
				}
				const auto maxAnisotropy = read<float>();
				const auto useHardwareMipMapGeneration = read<bool>();
				const auto unRefImageDataAfterApply = read<bool>();
				const auto clientStorageHint = read<bool>();
				const auto resizeNonPowerOfTwoHint = read<bool>();
				double borderColor[4]; read(borderColor, 4);
				const auto borderWidth = read<int>();
				const auto internalFormatMode = read<int>();
				if (read<bool>()) {
					const auto internalFormat = read<unsigned int>();
				}
				if (read<bool>()) {
					const auto sourceFormat = read<unsigned int>();
				}
				if (read<bool>()) {
					const auto sourceType = read<unsigned int>();
				}
				const auto shadowComparison = read<bool>();
				const auto shadowComparisonFunc = read<unsigned int>();
				const auto shadowTextureMode = read<unsigned int>();
				const auto shadowAmbient = read<float>();
				if ((_version >= 95) && (_version < 154) && read<bool>()) {
					int dummy[6]; read(dummy, 6);
				}
				if ((_version >= 98) && read<bool>()) {
					const auto swizzle = read<std::string>();
				}
				if (_version >= 155) {
					const auto minLOD = read<float>();
					const auto maxLOD = read<float>();
					const auto lodBias = read<float>();
				}
			}

			template<> void readObjectFields<Texture2D>(Texture2D& obj) {
				obj.image = readImage();
				const auto textureWidth = read<unsigned int>();
				const auto textureHeight = read<unsigned int>();
			}

			template<> void readObjectFields<DefaultUserDataContainer>(DefaultUserDataContainer& obj) {
				if (read<bool>()) { // UserData
					ReadBeginBracket();
					readObject();
					ReadEndBracket();
				}
				if (read<bool>()) { // Descriptions
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						read<std::string>();
					}
					ReadEndBracket();
				}
				if (read<bool>()) { // UserObjects;
					const auto size = read<unsigned int>();
					ReadBeginBracket();
					for (unsigned int i = 0; i < size; ++i) {
						readObject();
					}
					ReadEndBracket();
				}
			}

			template<> void readObjectFields<Array>(Array& obj) {
				obj.binding = read<Array::Binding>();
				obj.normalize = read<bool>();
				read<bool>(); // PreserveDataType
				obj.elementCount = read<unsigned int>();
				obj.elementData = _buffer + _pos;
			}

			template<> void readObjectFields<Vec2Array>(Vec2Array& obj) {
				_pos += obj.elementCount * sizeof(float) * 2;
			}

			template<> void readObjectFields<Vec3Array>(Vec3Array& obj) {
				_pos += obj.elementCount * sizeof(float) * 3;
			}

			std::unordered_map<unsigned int, std::shared_ptr<Object>> _objects;
			std::shared_ptr<Object> readObject() {
				const auto className = read<std::string>();
				if (className.empty()) {
					return nullptr;
				}
				ReadBeginBracket();
				const auto uniqueId = read<unsigned int>();
				for (const auto it = _objects.find(uniqueId); it != _objects.end();) {
					return it->second;
				}

				std::shared_ptr<Object> object;
				if (className == "osg::PagedLOD") {
					object = readObjectData<PagedLOD>();
				} else if (className == "osg::Group") {
					object = readObjectData<Group>();
				} else if (className == "osg::Geode") {
					object = readObjectData<Geode>();
				} else if (className == "osg::Geometry") {
					object = readObjectData<Geometry>();
				} else if (className == "osg::StateSet") {
					object = readObjectData<StateSet>();
				} else if (className == "osg::Material") {
					object = readObjectData<Material>();
				} else if (className == "osg::Texture2D") {
					object = readObjectData<Texture2D>();
				} else if (className == "osg::DefaultUserDataContainer") {
					object = readObjectData<DefaultUserDataContainer>();
				} else if (className == "osg::DrawElementsUInt") {
					object = readObjectData<DrawElementsUInt>();
				} else if (className == "osg::Vec3Array") {
					object = readObjectData<Vec3Array>();
				} else if (className == "osg::Vec2Array") {
					object = readObjectData<Vec2Array>();
				} else {
					throw Error(_pos, "unsupported object class: " + className);
				}
				ReadEndBracket();

				if (object) {
					object->uniqueId = uniqueId;
					_objects[uniqueId] = object;
				}
				return object;
			}

			std::unordered_map<unsigned int, std::shared_ptr<Image>> _images;
			std::shared_ptr<Image> readImage() {
				// InputStream::ReadImage() https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/src/osgDB/InputStream.cpp
				if (read<bool>()) {
					if (_version > 94) {
						const auto className = read<std::string>();
					}
					const auto uniqueId = read<unsigned int>();
					for (const auto it = _images.find(uniqueId); it != _images.end();) {
						return it->second;
					}

					auto image = std::make_shared<Image>();
					image->uniqueId = uniqueId;
					_images[uniqueId] = image;

					const auto name = read<std::string>();
					const auto writeHint = read<unsigned int>();
					const auto decision = read<unsigned int>();
					if (decision == 1) { // IMAGE_INLINE_FILE 
						const auto size = read<unsigned int>();
						image->data = _buffer + _pos;
						image->dataLength = size;
						//{ // DEBUG
						//	FILE* file = nullptr;
						//	fopen_s(&file, "D:\\tile.png", "wb+");
						//	if (file) {
						//		fwrite(image->data, 1, image->dataLength, file);
						//		fclose(file);
						//	}
						//}
						this->_pos += size;
					} else {
						// 0 => IMAGE_INLINE_DATA
						// 2 => IMAGE_EXTERNAL
						throw Error(_pos, "invalid image decision: " + std::to_string(decision));
					}
					readObjectFields<Object>(*image);
					return image;
				} else {
					return {};
				}
			}

			std::unordered_map<unsigned int, std::shared_ptr<Array>> _arrays;
			std::shared_ptr<Array> ReadArray() {
				if (read<bool>()) { // hasArray
					const auto uniqueId = read<unsigned int>();
					for (const auto it = _arrays.find(uniqueId); it != _arrays.end();) {
						return it->second;
					}
					std::shared_ptr<Array> arr;

					//https://github.com/openscenegraph/OpenSceneGraph/blob/OpenSceneGraph-3.6/include/osgDB/DataTypes
					const auto type = read<int>();
					switch (type)
					{
						case 15: // ID_VEC2_ARRAY 
							arr = std::make_shared<Vec2Array>();
							break;
						case 16: // ID_VEC3_ARRAY 
							arr = std::make_shared<Vec3Array>();
							break;
						case 17: // ID_VEC4_ARRAY
							arr = std::make_shared<Vec4Array>();
							break;
						default:
							throw Error(_pos, "unsupported array type: " + std::to_string(type));
					}

					arr->uniqueId = uniqueId;
					_arrays[uniqueId] = arr;

					const auto elementCount = read<unsigned int>();
					arr->elementCount = elementCount;
					if (elementCount > 0) {
						arr->elementData = _buffer + _pos;
					}
					_pos += elementCount * arr->elementSize;
					if (read<bool>()) { // hasIndices
						//not supported
						throw Error(_pos, "unsupported feature: array with indices");
					}
					arr->binding = read<Array::Binding>();
					arr->normalize = (read<unsigned int>() != 0);
					return arr;
				}
				return nullptr;
			}

			void ReadBeginBracket() {
				if (_useBinaryBrackets) {
					if (_version > 148) {
						_pos += sizeof(long long);
					} else {
						_pos += sizeof(int);
					}
				}
			}
			void ReadEndBracket() {}

			void readHeader() {
				if (read<long long>() != 0x1AFB45456C910EA1) {
					throw Error(_pos, "invalid data magic");
				}

				// 0: Unknown, 1: Scene, 2: Image, 3: Object
				const auto type = read<unsigned int>();
				if (type == 0) {
					throw Error(_pos, "invalid data type: " + std::to_string(type));
				}

				_version = read<unsigned int>();

				// 0x01: custom domains
				// 0x02: use schema data 
				// 0x04: support binary brackets
				const auto attributes = read<unsigned int>();
				if ((attributes & 0x01) || (attributes & 0x02)) {
					throw Error(_pos, "unsupported attribute: " + std::to_string(type));
				}
				_useBinaryBrackets = ((attributes & 0x04) != 0);

				const auto compressorName = read<std::string>();
				if (compressorName != "0") {
					throw Error(_pos, "unsupported compressor: " + compressorName);
				}
			}
		};
	}

	struct Data {
		std::shared_ptr<Object> rootObject;

		static std::unique_ptr<Data> read(const unsigned char* buffer, size_t length, std::string* error = nullptr)
		{
#ifndef _DEBUG
			try {
#endif
				details::Reader reader(buffer, length);
				reader.readHeader();

				auto data = std::make_unique<Data>();
				data->rootObject = reader.readObject();
				if (data->rootObject && reader.ended()) {
					return data;
				} else {
					return nullptr;
				}
#ifndef _DEBUG
			} catch (const details::Reader::Error& ex) {
				if (error) {
					*error = "miniosgb reader error at offset " + std::to_string(ex.offset) + ": " + ex.what();
				}
				return nullptr;
			}
#endif
		}
	};
};

