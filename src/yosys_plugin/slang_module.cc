
#include "slang_module.h"
#include "driver.h"

#include <string>

namespace Yosys {

// create a new parametric module (when needed) and return the name of the generated module - without support for interfaces
RTLIL::IdString SlangModule::derive(RTLIL::Design *design, const dict<RTLIL::IdString, RTLIL::Const> &parameters, bool /*mayfail*/)
{
	std::vector<std::string> overrides;

	for (const auto &[key, value] : parameters) {
		std::string name = slang_frontend::unescaped_id(design, key);
		std::string literal = convert_const_slang(value).toString(slang::SVInt::MAX_BITS, true);
		overrides.push_back(name + "=" + literal);
	}

	IdString original_name = this->name;
	IdString modname = original_name;
	if (parameters.size())
		modname = slang_frontend::intern_id(design, compute_module_name(this->name.str(), overrides));

	if (design->has(modname))
		return modname;

	this->settings.driver->options.paramOverrides = std::move(overrides);
	elaborate(design, slang_frontend::ModuleRename{original_name, modname}, this->settings);

	return modname;
}

// create a new parametric module (when needed) and return the name of the generated module - WITH support for interfaces
RTLIL::IdString SlangModule::derive(RTLIL::Design *design, const dict<RTLIL::IdString, RTLIL::Const> &parameters, [[maybe_unused]] const dict<RTLIL::IdString, RTLIL::Module*> &interfaces, [[maybe_unused]] const dict<RTLIL::IdString, RTLIL::IdString> &modports, bool /*mayfail*/)
{
	bool mayfail = false;
	return derive(design, parameters, mayfail);
}

RTLIL::Module *SlangModule::clone() const
{
	SlangModule *new_mod = new SlangModule(design);
	new_mod->name = name;
	cloneInto(new_mod);
	copy_config_into(new_mod);

	return new_mod;
}

#if YOSYS_TWINES
RTLIL::Module *SlangModule::clone(RTLIL::Design *dst) const
{
	return clone(dst, dst->twines().copy_from(design->twines(), name));
}

RTLIL::Module *SlangModule::clone(RTLIL::Design *dst, IdString target_name) const
{
	SlangModule *new_mod = new SlangModule(dst);
	new_mod->name = target_name;
	cloneInto(new_mod);
	dst->add(new_mod);
	copy_config_into(new_mod);

	return new_mod;
}
#endif

void SlangModule::copy_config_into(SlangModule *new_mod) const
{
	new_mod->settings = this->settings;
}

std::string compute_module_name(std::string name, std::vector<std::string> params) {
	std::string para_info;
	for (const auto &param : params)
		para_info += stringf("%s", param);
	return "$paramod" + name + para_info;
}

const slang::ConstantValue convert_const_slang(const RTLIL::Const &constval) {
	// Real Convert
	if (constval.flags & RTLIL::CONST_FLAG_REAL) {
		auto text = constval.decode_string();
		size_t consumed = 0;
		std::optional<double> value = slang::strToDouble(text, &consumed);
		if (value && consumed == text.size())
			return slang::ConstantValue(slang::real_t(*value));
		log_error("Cannot convert constant: %s to slang real", text);
	}

	// String Convert
	if (constval.flags & RTLIL::CONST_FLAG_STRING)
		return slang::ConstantValue(constval.decode_string());

	// Bits Convert
	auto rtlil_to_slang = [](RTLIL::State bit, const RTLIL::Const& constval, bool& any_unknown) {
		switch (bit) {
		case RTLIL::S0: return slang::logic_t(0);
		case RTLIL::S1: return slang::logic_t(1);
		case RTLIL::Sx:
			any_unknown = true;
			return slang::logic_t::x;
			break;
		case RTLIL::Sz:
			any_unknown = true;
			return slang::logic_t::z;
			break;
		default:
			log_error("Cannot convert don't care or marker bits in: %s to slang bits", constval.decode_string());
		}
	};
	std::vector<slang::logic_t> digits;
	digits.reserve(constval.size());
	bool any_unknown = false;
	for (int i = constval.size() - 1; i >= 0; i--) {
		digits.emplace_back(rtlil_to_slang(constval[i], constval, any_unknown));
	}

	return slang::ConstantValue(slang::SVInt::fromDigits(constval.size(),
			slang::LiteralBase::Binary, constval.flags & RTLIL::CONST_FLAG_SIGNED,
			any_unknown, digits));
}

} // namespace Yosys
