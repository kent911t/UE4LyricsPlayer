// Fill out your copyright notice in the Description page of Project Settings.


#include "LyricsBlueprintFunctionLibrary.h"
#include "Components/AudioComponent.h"

float ULyricsBlueprintFunctionLibrary::LRCTimeToSeconds(FString Time)
{
	const FRegexPattern TimeFormatPattern("(\\d+):([012345]\\d).(\\d+)");
	FRegexMatcher TimeMatcher(TimeFormatPattern, Time);

	while (TimeMatcher.FindNext())
	{
		FString Minutes = TimeMatcher.GetCaptureGroup(1);
		FString Seconds = TimeMatcher.GetCaptureGroup(2);
		FString MSeconds = TimeMatcher.GetCaptureGroup(3);
		if (Minutes.IsNumeric() && Seconds.IsNumeric() && MSeconds.IsNumeric())
		{
			float mins = FCString::Atof(*Minutes);
			float secs = FCString::Atof(*Seconds);
			float ms = FCString::Atof(*MSeconds);

			//UE_LOG(LogTemp, Warning, TEXT("Time: %f:%f:%f\nConverted Time: %f"), (mins * 60) + (secs) + (ms / 100));

			return (mins * 60) + (secs) + (ms / 100);
		}
	}

	return 0.0f;
}

FLyricLine ULyricsBlueprintFunctionLibrary::FormatLyricLine(FString Line)
{
	const FRegexPattern FirstWordPattern("\\[([0-9]{2}\\:[0-9]{2}\\.[0-9]{2})?\\]([^:\\s]+)");
	const FRegexPattern WordPattern("\\<([0-9]{2}\\:[0-9]{2}\\.[0-9]{2})?\\>([^:\\s]+)");

	FRegexMatcher FirstWordMatcher(FirstWordPattern, Line);
	FRegexMatcher WordMatcher(WordPattern, Line);

	FLyricLine LyricLine;
	while (FirstWordMatcher.FindNext())
	{
		FLyricWord NewWord;
		NewWord.Time = LRCTimeToSeconds(FirstWordMatcher.GetCaptureGroup(1));
		NewWord.Word = FirstWordMatcher.GetCaptureGroup(2);
		LyricLine.Words.Add(NewWord);

		while (WordMatcher.FindNext())
		{
			FLyricWord Word;
			Word.Time = LRCTimeToSeconds(WordMatcher.GetCaptureGroup(1));
			Word.Word = WordMatcher.GetCaptureGroup(2);
			LyricLine.Words.Add(Word);
		}
	}

	return LyricLine;
}

void ULyricsBlueprintFunctionLibrary::IsLyricHit(const USoundWave* PlayingSoundWave, const float PlaybackPercent, const FLyricFileStruct LyricStruct, UPARAM(ref) int& LineCounter, UPARAM(ref) int& WordCounter, bool& Hit, FString& WordString) {
	//UE_LOG(LogTemp, Warning, TEXT("Line: %d \nWord: %d\nWords In Line: %d"), LineCounter, WordCounter, LyricStruct.Lines[LineCounter].Words.Num())
	// We've reached final line
	Hit = false;
	if (LyricStruct.Lines.Num()-1 < LineCounter)
	{
		LineCounter = 0;

		return;
	}
	else
	{
		TArray<FLyricWord> Line = LyricStruct.Lines[LineCounter].Words;

		// We've reached the final word
		if (Line.Num()-1 < WordCounter)
		{
			WordCounter = 0;
			++LineCounter;

			return;
		}
		else
		{
			// Play a word
			float CurrentTime = PlayingSoundWave->Duration * PlaybackPercent;
			FLyricWord CurrentWord = Line[WordCounter];

			if (CurrentTime > CurrentWord.Time)
			{
				++WordCounter;
				WordString = CurrentWord.Word;

				Hit = true;
			}

			return;
		}
	}
}

void ULyricsBlueprintFunctionLibrary::ParseLyrics(const FString ELRCFile, FLyricFileStruct& LyricStruct)
{
	TArray<FString> LineArray;
	ELRCFile.ParseIntoArray(LineArray, TEXT("\n"), false);
	//https://stackoverflow.com/questions/14831484/validate-with-regex-for-lrc-file-in-javascript
	
	for (auto& Line : LineArray)
	{
		const FRegexPattern TagPattern("\\[([a-z]+)\\:(.+)\\]");
		FRegexMatcher TagValueMatcher(TagPattern, Line);

		// Add tags
		while (TagValueMatcher.FindNext())
		{
			FLyricTag NewTag;
			NewTag.Tag = TagValueMatcher.GetCaptureGroup(1);
			NewTag.Value = TagValueMatcher.GetCaptureGroup(2);
			LyricStruct.Tags.Add(NewTag);
		}

		// Add lyrics
		FLyricLine NewLyricLine = FormatLyricLine(Line);
		if (NewLyricLine.Words.Num() > 0)
		{
			LyricStruct.Lines.Add(NewLyricLine);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("%s"), *LyricStruct.Lines[0].Words[0].Word);
}
