#include "../includes/luaresolver.hpp"
#include <iostream>
#include <cstdio>
#include <type_traits>

void testShortSeek() {
	try {
		Resolver r("config.lua");
		// short path: direct name#type match
		if (auto v = r.get<String>("name#s"); v != std::nullopt) std::cout << "short name#s => " << *v << std::endl;
		if (auto v = r.get<double>("version#d"); v != std::nullopt) std::cout << "short version#d => " << *v << std::endl;
		if (auto v = r.get<bool>("x#b"); v != std::nullopt) std::cout << "short x#b => " << *v << std::endl;
		if (auto v = r.get<String>("s#s"); v != std::nullopt) std::cout << "short s#s => " << *v << std::endl;
		if (auto v = r.get<String>("y#s"); v != std::nullopt) std::cout << "short y#s => " << *v << std::endl;
		if (auto v = r.get<int>("1#i"); v != std::nullopt) std::cout << "short 1#i => " << *v << std::endl;
		if (auto v = r.get<String>("host#s"); v != std::nullopt) std::cout << "short host#s => " << *v << std::endl;

		if (auto v = r.get<Node*>("flags#t"); v != std::nullopt) std::cout << "short flags#s => " << "getTable" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "testShortSeek error: " << e.what() << std::endl;
	}
}

void testAbsSeek() {
	try {
		Resolver r("config.lua");
		// absolute path: root omitted, chain of name#type>name#type
		if (auto v = r.absGet<String>("config#t>app#t>name#s"); v != std::nullopt) std::cout << "abs config#t>app#t>name#s => " << *v << std::endl;
		if (auto v = r.absGet<double>("config#t>app#t>version#d"); v != std::nullopt) std::cout << "abs config#t>app#t>version#d => " << *v << std::endl;
		if (auto v = r.absGet<double>("config#t>app#t>thresholds#t>2#d"); v != std::nullopt) std::cout << "abs thresholds[2] => " << *v << std::endl;

		if (auto v = r.absGet<Node*>("config#t>app#t>list_of_maps#t>2#t>flags#t"); v != std::nullopt) std::cout << "abs config#t>app#t>list_of_maps#t>flags#t => " << "getTable" << std::endl;

		if (auto v = r.absGet<bool>("config#t>app#t>nested#t>b#t>x#b"); v != std::nullopt) std::cout << "abs config#t>app#t>nested#t>b#t>x#b => " << *v << std::endl;
		if (auto v = r.absGet<int>("config#t>app#t>thresholds#t>3#i"); v != std::nullopt) std::cout << "abs config#t>app#t>thresholds#t>3#i => " << *v << std::endl;
		if (auto v = r.absGet<String>("config#t>servers#t>2#t>host#s"); v != std::nullopt) std::cout << "abs config#t>servers#t>2#t>host#s => " << *v << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "testAbsSeek error: " << e.what() << std::endl;
	}
}


void dumpNode(Node& node, char gp, char ty, int depth = 0) {
	std::string indent(depth * 2, ' ');
	for (auto& [k, v] : node.mulitdict) {
		String key = SpiltPrefix(k, gp);
		printf("%sVAL %s = ", indent.c_str(), key.c_str());
		std::visit([&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, int>) printf("%d", arg);
			else if constexpr (std::is_same_v<T, double>) printf("%f", arg);
			else if constexpr (std::is_same_v<T, String>) printf("%s", arg.c_str());
			else if constexpr (std::is_same_v<T, bool>) printf(arg ? "true" : "false");
			}, v);
		printf("\n");
	}
	for (auto& [k, ptr] : node.recnode) {
		String key = SpiltPrefix(k, gp);
		printf("%sTABLE %s:\n", indent.c_str(), key.c_str());
		dumpNode(ptr.RefRaw(), gp, ty, depth + 1);
	}
}

void dumpAll(Node& root) {
	dumpNode(root, /*sign[0], sign[1]*/'>', '#');
}


// ==== launch ==== //
int test() {
	try {
		Resolver r("config.lua");
		dumpAll(r.RefRaw());
		// run short and absolute seek tests
		testShortSeek();
		testAbsSeek();
	}
	catch (const std::exception& e) {
		std::cerr << "Resolver error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
