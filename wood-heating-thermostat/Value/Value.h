
template <typename T, typename U = int, U default_u = 0>
class Value {
	volatile T val;
	volatile U _origin;
	volatile bool changed;
	bool constrained;
	T constrain_min;
	T constrain_max;

	public:
	Value(bool changed = false): changed(changed), constrained(false) {}
	
	Value(T initial_val, bool changed = false): val(initial_val), changed(changed), constrained(false) {}
	
	Value(T initial_val, T min_val, T max_val, bool changed = false): changed(changed), constrained(true) {
		val				= constrain(initial_val, min_val, max_val);
		constrain_min	= min_val;
		constrain_max	= max_val;
	}
	
	T get() const {
		return val;
	}
	
	void set(T new_val, U origin = default_u) {

		if (constrained) {
			new_val = constrain(new_val, constrain_min, constrain_max);
		}

		if (val != new_val) {
			changed = true;
			_origin = origin;
		}
		val = new_val;
	}
	
	void inc(U origin = default_u) {
		set( get() + 1, origin );
	}
	
	void dec(U origin = default_u) {
		set( get() - 1, origin );
	}
	
	U origin() {
		return _origin;
	}

	bool change() {
		if (changed) {
			changed = false;
			return true;
		}
		return false;
	}

	/**
	* set and check for a change
	*/
	bool change(T new_val) {
		set(new_val);
		return change();
	}
	/*
	Value<T, U>& operator=(const Value<T, U>& other) {
		Serial.println("assignment");
		if (&other == this) {
			return *this;
		}
		changed = false;
		constrained		= other.constrained;
		constrain_min	= other.constrain_min;
		constrain_max	= other.constrain_max;
		set( other.get() );
		_origin			= other._origin;
	}*/
};
