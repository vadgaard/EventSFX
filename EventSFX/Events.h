#pragma once

namespace RlEvents
{
	class Kind
	{
	public:
		enum EventType
			: std::uint8_t
		{
			Bump = 0,
			Demo,
			Crossbar,
			Win,
			Loss,
			PlayerGoal,
			TeamGoal,
			Concede,
			Save,
			Assist
		};

		Kind(const EventType t) : Value(t)
		{}

		explicit Kind(int t) : Value(static_cast<EventType>(t))
		{}

		operator int() const
		{
			return this->Value;
		}

	private:
		EventType Value;
	};

	inline bool IsEvent3D(const Kind eventId)
	{
		return static_cast<int>(eventId) < 3;
	}

	inline std::string GetEventLabel(const Kind eventId)
	{
		switch (eventId)
		{
		case Kind::Bump:
			return "bump";
		case Kind::Demo:
			return "demo";
		case Kind::Crossbar:
			return "crossbar";
		case Kind::Win:
			return "win";
		case Kind::Loss:
			return "loss";
		case Kind::PlayerGoal:
			return "player goal";
		case Kind::TeamGoal:
			return "team goal";
		case Kind::Concede:
			return "concede";
		case Kind::Save:
			return "save";
		case Kind::Assist:
			return "assist";
		default:
			return "NA";
		}
	}

	inline std::string GetEventStringId(const Kind eventId)
	{
		switch (eventId)
		{
		case Kind::PlayerGoal:
			return "playergoal";
		case Kind::TeamGoal:
			return "teamgoal";
		default:
			return GetEventLabel(eventId);
		}
	}
}
