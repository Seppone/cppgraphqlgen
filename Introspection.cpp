// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <graphqlservice/Introspection.h>

namespace facebook {
namespace graphql {
namespace introspection {

Schema::Schema()
{
}

void Schema::AddQueryType(std::shared_ptr<ObjectType> query)
{
	_query = std::move(query);
}

void Schema::AddMutationType(std::shared_ptr<ObjectType> mutation)
{
	_mutation = std::move(mutation);
}

void Schema::AddSubscriptionType(std::shared_ptr<ObjectType> subscription)
{
	_subscription = std::move(subscription);
}

void Schema::AddType(response::StringType name, std::shared_ptr<object::__Type> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ std::move(name), std::move(type) });
}

const std::shared_ptr<object::__Type>& Schema::LookupType(const response::StringType& name) const
{
	return _types[_typeMap.find(name)->second].second;
}

const std::shared_ptr<object::__Type>& Schema::WrapType(__TypeKind kind, const std::shared_ptr<object::__Type>& ofType)
{
	auto& wrappers = (kind == __TypeKind::LIST)
		? _listWrappers
		: _nonNullWrappers;
	auto itr = wrappers.find(ofType);

	if (itr == wrappers.cend())
	{
		std::tie(itr, std::ignore) = wrappers.insert({ ofType, std::make_shared<WrapperType>(kind, ofType) });
	}

	return itr->second;
}

void Schema::AddDirective(std::shared_ptr<object::__Directive> directive)
{
	_directives.emplace_back(std::move(directive));
}

std::future<std::vector<std::shared_ptr<object::__Type>>> Schema::getTypes(service::FieldParams&&) const
{
	auto spThis = shared_from_this();

	return std::async(std::launch::deferred,
		[this, spThis]()
	{
		std::vector<std::shared_ptr<object::__Type>> result(_types.size());

		std::transform(_types.cbegin(), _types.cend(), result.begin(),
			[](const std::pair<response::StringType, std::shared_ptr<object::__Type>>& namedType)
		{
			return namedType.second;
		});

		return result;
	});
}

std::future<std::shared_ptr<object::__Type>> Schema::getQueryType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_query);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getMutationType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_mutation);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getSubscriptionType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_subscription);

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__Directive>>> Schema::getDirectives(service::FieldParams&&) const
{
	std::promise<std::vector<std::shared_ptr<object::__Directive>>> promise;

	promise.set_value(_directives);

	return promise.get_future();
}

BaseType::BaseType(response::StringType description)
	: _description(std::move(description))
{
}

std::future<std::unique_ptr<response::StringType>> BaseType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> BaseType::getDescription(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> BaseType::getFields(service::FieldParams&&, std::unique_ptr<response::BooleanType>&& /*includeDeprecatedArg*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getInterfaces(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getPossibleTypes(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> BaseType::getEnumValues(service::FieldParams&&, std::unique_ptr<response::BooleanType>&& /*includeDeprecatedArg*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> BaseType::getInputFields(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> BaseType::getOfType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

ScalarType::ScalarType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

std::future<__TypeKind> ScalarType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::SCALAR);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> ScalarType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

ObjectType::ObjectType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void ObjectType::AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces)
{
	_interfaces = std::move(interfaces);
}

void ObjectType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::future<__TypeKind> ObjectType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> ObjectType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> ObjectType::getFields(service::FieldParams&& params, std::unique_ptr<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> ObjectType::getInterfaces(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_interfaces.size()));

	std::copy(_interfaces.cbegin(), _interfaces.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

InterfaceType::InterfaceType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::future<__TypeKind> InterfaceType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INTERFACE);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InterfaceType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> InterfaceType::getFields(service::FieldParams&& params, std::unique_ptr<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

UnionType::UnionType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void UnionType::AddPossibleTypes(std::vector<std::weak_ptr<object::__Type>> possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
}

std::future<__TypeKind> UnionType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::UNION);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> UnionType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> UnionType::getPossibleTypes(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_possibleTypes.size()));

	std::transform(_possibleTypes.cbegin(), _possibleTypes.cend(), result->begin(),
		[](const std::weak_ptr<object::__Type>& weak)
	{
		return weak.lock();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

EnumType::EnumType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void EnumType::AddEnumValues(std::vector<EnumValueType> enumValues)
{
	_enumValues.reserve(_enumValues.size() + enumValues.size());

	for (auto& value : enumValues)
	{
		_enumValues.push_back(std::make_shared<EnumValue>(std::move(value.value),
			std::move(value.description),
			std::unique_ptr<response::StringType>(value.deprecationReason
				? new response::StringType(value.deprecationReason)
				: nullptr)));
	}
}

std::future<__TypeKind> EnumType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::ENUM);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> EnumType::getEnumValues(service::FieldParams&& params, std::unique_ptr<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> result(new std::vector<std::shared_ptr<object::__EnumValue>>());

	result->reserve(_enumValues.size());
	std::copy_if(_enumValues.cbegin(), _enumValues.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<object::__EnumValue>& value)
	{
		return deprecated
			|| !value->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

InputObjectType::InputObjectType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

std::future<__TypeKind> InputObjectType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INPUT_OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputObjectType::getName(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> InputObjectType::getInputFields(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> result(new std::vector<std::shared_ptr<object::__InputValue>>(_inputValues.size()));

	std::copy(_inputValues.cbegin(), _inputValues.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

WrapperType::WrapperType(__TypeKind kind, const std::shared_ptr<object::__Type>& ofType)
	: BaseType(response::StringType())
	, _kind(kind)
	, _ofType(ofType)
{
}

std::future<__TypeKind> WrapperType::getKind(service::FieldParams&&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(_kind);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> WrapperType::getOfType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_ofType.lock());

	return promise.get_future();
}

Field::Field(response::StringType name, response::StringType description, std::unique_ptr<response::StringType>&& deprecationReason, std::vector<std::shared_ptr<InputValue>> args, const std::shared_ptr<object::__Type>& type)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
	, _args(std::move(args))
	, _type(type)
{
}

std::future<response::StringType> Field::getName(service::FieldParams&&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Field::getDescription(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__InputValue>>> Field::getArgs(service::FieldParams&&) const
{
	std::promise<std::vector<std::shared_ptr<object::__InputValue>>> promise;
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Field::getType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<response::BooleanType> Field::getIsDeprecated(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Field::getDeprecationReason(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<response::StringType>(new response::StringType(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

InputValue::InputValue(response::StringType name, response::StringType description, const std::shared_ptr<object::__Type>& type, response::StringType defaultValue)
	: _name(std::move(name))
	, _description(std::move(description))
	, _type(type)
	, _defaultValue(std::move(defaultValue))
{
}

std::future<response::StringType> InputValue::getName(service::FieldParams&&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputValue::getDescription(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> InputValue::getType(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputValue::getDefaultValue(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_defaultValue.empty()
		? nullptr
		: new response::StringType(_defaultValue)));

	return promise.get_future();
}

EnumValue::EnumValue(response::StringType name, response::StringType description, std::unique_ptr<response::StringType>&& deprecationReason)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
{
}

std::future<response::StringType> EnumValue::getName(service::FieldParams&&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumValue::getDescription(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<response::BooleanType> EnumValue::getIsDeprecated(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumValue::getDeprecationReason(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<response::StringType>(new response::StringType(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

Directive::Directive(response::StringType name, response::StringType description, std::vector<response::StringType> locations, std::vector<std::shared_ptr<InputValue>> args)
	: _name(std::move(name))
	, _description(std::move(description))
	, _locations([](std::vector<response::StringType>&& locationsArg) -> std::vector<__DirectiveLocation>
		{
			std::vector<__DirectiveLocation> result(locationsArg.size());

			std::transform(locationsArg.begin(), locationsArg.end(), result.begin(),
				[](std::string& name) -> __DirectiveLocation
				{
					response::Value location(response::Type::EnumValue);

					location.set<response::StringType>(std::move(name));
					return service::ModifiedArgument<__DirectiveLocation>::convert(location);
				});

			return result;
		}(std::move(locations)))
	, _args(std::move(args))
{
}

std::future<response::StringType> Directive::getName(service::FieldParams&&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Directive::getDescription(service::FieldParams&&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::vector<__DirectiveLocation>> Directive::getLocations(service::FieldParams&&) const
{
	std::promise<std::vector<__DirectiveLocation>> promise;
	std::vector< __DirectiveLocation> result(_locations.size());

	std::copy(_locations.cbegin(), _locations.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__InputValue>>> Directive::getArgs(service::FieldParams&&) const
{
	std::promise<std::vector<std::shared_ptr<object::__InputValue>>> promise;
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */
