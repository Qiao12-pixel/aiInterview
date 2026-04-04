//
// Created by Joe on 26-4-4.
//

#ifndef INTERVIEW_STATE_H
#define INTERVIEW_STATE_H



namespace interview {
    namespace common {
        enum class InterviewState {
            kConnecting = 0,
            kInterviewSpeaking,
            kIdle,
            kCandidateSpeaking,
            kInterviewerThinking,
            kSessionEnding,
            kCompleted,
            kError,
        };
    }
}



#endif //INTERVIEW_STATE_H
