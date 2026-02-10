config = {
	config = {
		app = {
			name = "tchains",
			version = 2.9,
			enabled = true,
			thresholds = { 10, 20.5, 30 },
			nested = {
				a = 1,
				b = {
					x = false,
					y = "yes",
					z = { 1, 2, { inner = "deep" } },
                    s = "string",
				},
			},
			list_of_maps = {
				{ id = 1, name = "one" },
				{ id = 2, name = "two", flags = { true, false } },
			},
		},
		servers = {
			{ host = "127.0.0.1", port = 8080 },
			{ host = "10.0.0.1", port = 80 },
		},
		values = { [1] = "one", [2] = "two", [3] = 3 },
		empty_table = {},
		misc = {
			negative = -42,
			float_as_int = 42.0,
			mixed_keys = { ["complex-key"] = "ok", [10] = "ten" },
		},
        name = "interrupted",
	}
}

return config