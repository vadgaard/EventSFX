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
		return static_cast<int>(eventId) < 2;
	}

	inline std::string GetPlural(const Kind eventId)
	{
		switch (eventId)
		{
		case Kind::Bump:
			return "bumps";
		case Kind::Demo:
			return "demos";
		case Kind::Win:
			return "wins";
		case Kind::Loss:
			return "losses";
		case Kind::PlayerGoal:
			return "player goals";
		case Kind::TeamGoal:
			return "team goals";
		case Kind::Concede:
			return "concedes";
		case Kind::Save:
			return "saves";
		case Kind::Assist:
			return "assists";
		default:
			return "NA";
		}
	}

	inline std::string GetSingular(const Kind eventId)
	{
		std::string singular = GetPlural(eventId);
		switch (eventId)
		{
		case Kind::Loss:
			singular.pop_back();
			[[fallthrough]];
		default:
			singular.pop_back();
			return singular;
		}
	}

	inline std::string GetPluralStringId(const Kind eventId)
	{
		switch (eventId)
		{
		case Kind::PlayerGoal:
			return "playergoals";
		case Kind::TeamGoal:
			return "teamgoals";
		default:
			return GetPlural(eventId);
		}
	}

	inline std::string GetSingularStringId(const Kind eventId)
	{
		switch (eventId)
		{
		case Kind::PlayerGoal:
			return "playergoal";
		case Kind::TeamGoal:
			return "teamgoal";
		default:
			return GetSingular(eventId);
		}
	}
}
