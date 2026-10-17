#核心逻辑
# chat_logic.py

import string
from qa_data import QA_PAIRS

class ChatProcessor:
    @staticmethod
    def preprocess(text):
        translator = str.maketrans('', '', string.punctuation + '！？｡。＂＃＄％＆＇（）＊＋，－／：；＜＝＞＠［＼］＾＿｀｛｜｝～')
        return text.translate(translator).strip()

    def fuzzy_match(self, user_input):
        user_chars = set(user_input)
        best_match = None
        max_overlap = 0

        for question in QA_PAIRS:
            question_chars = set(question)
            overlap = len(user_chars & question_chars)
            threshold = max(3, len(question_chars) // 2)

            if overlap >= threshold and overlap > max_overlap:
                max_overlap = overlap
                best_match = question

        return best_match if max_overlap >= 3 else None

    def get_response(self, user_text):
        cleaned_text = self.preprocess(user_text)
        return QA_PAIRS.get(cleaned_text) or QA_PAIRS.get(self.fuzzy_match(cleaned_text), "暂时不知道，我会继续学习")