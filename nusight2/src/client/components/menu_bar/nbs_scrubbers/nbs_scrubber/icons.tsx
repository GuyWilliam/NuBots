import React from "react";

export function IconPlay(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M9.875 17.225Q9.425 17.525 8.963 17.25Q8.5 16.975 8.5 16.45V7.55Q8.5 7.025 8.963 6.75Q9.425 6.475 9.875 6.775L16.875 11.225Q17.3 11.5 17.3 12Q17.3 12.5 16.875 12.775ZM10 12ZM10 15.35 15.275 12 10 8.65Z" />
    </svg>
  );
}

export function IconPause(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M14.75 18.5Q14.125 18.5 13.688 18.062Q13.25 17.625 13.25 17V7Q13.25 6.375 13.688 5.938Q14.125 5.5 14.75 5.5H17Q17.625 5.5 18.062 5.938Q18.5 6.375 18.5 7V17Q18.5 17.625 18.062 18.062Q17.625 18.5 17 18.5ZM7 18.5Q6.375 18.5 5.938 18.062Q5.5 17.625 5.5 17V7Q5.5 6.375 5.938 5.938Q6.375 5.5 7 5.5H9.25Q9.875 5.5 10.312 5.938Q10.75 6.375 10.75 7V17Q10.75 17.625 10.312 18.062Q9.875 18.5 9.25 18.5ZM14.75 17H17V7H14.75ZM7 17H9.25V7H7ZM7 7V17ZM14.75 7V17Z" />
    </svg>
  );
}

export function IconRewind(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M19.3 16.375 13.875 12.75Q13.45 12.475 13.45 12Q13.45 11.525 13.875 11.25L19.3 7.625Q19.75 7.3 20.225 7.575Q20.7 7.85 20.7 8.375V15.625Q20.7 16.15 20.225 16.425Q19.75 16.7 19.3 16.375ZM9.85 16.375 4.425 12.75Q4 12.475 4 12Q4 11.525 4.425 11.25L9.85 7.625Q10.3 7.3 10.775 7.575Q11.25 7.85 11.25 8.375V15.625Q11.25 16.15 10.775 16.425Q10.3 16.7 9.85 16.375ZM9.75 12ZM19.2 12ZM9.75 14.5V9.5L6 12ZM19.2 14.5V9.5L15.425 12Z" />
    </svg>
  );
}

export function IconFastForward(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M4.7 16.375Q4.25 16.7 3.775 16.425Q3.3 16.15 3.3 15.625V8.375Q3.3 7.85 3.775 7.575Q4.25 7.3 4.7 7.625L10.125 11.25Q10.55 11.525 10.55 12Q10.55 12.475 10.125 12.75ZM14.15 16.375Q13.7 16.7 13.225 16.425Q12.75 16.15 12.75 15.625V8.375Q12.75 7.85 13.225 7.575Q13.7 7.3 14.15 7.625L19.575 11.25Q20 11.525 20 12Q20 12.475 19.575 12.75ZM4.8 12ZM14.25 12ZM4.8 14.5 8.575 12 4.8 9.5ZM14.25 14.5 18 12 14.25 9.5Z" />
    </svg>
  );
}

export function IconRepeat(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M17.15 17.15V13.9Q17.15 13.575 17.363 13.362Q17.575 13.15 17.9 13.15Q18.225 13.15 18.438 13.362Q18.65 13.575 18.65 13.875V17.75Q18.65 18.125 18.388 18.387Q18.125 18.65 17.75 18.65H6.325L7.6 19.925Q7.85 20.175 7.85 20.475Q7.85 20.775 7.625 21.025Q7.4 21.25 7.088 21.25Q6.775 21.25 6.575 21.05L4.075 18.525Q3.825 18.275 3.825 17.9Q3.825 17.525 4.075 17.275L6.55 14.8Q6.775 14.55 7.088 14.55Q7.4 14.55 7.625 14.8Q7.85 15.025 7.85 15.325Q7.85 15.625 7.625 15.85L6.325 17.15ZM6.85 6.85V10.1Q6.85 10.425 6.638 10.637Q6.425 10.85 6.1 10.85Q5.775 10.85 5.562 10.637Q5.35 10.425 5.35 10.125V6.25Q5.35 5.875 5.613 5.612Q5.875 5.35 6.25 5.35H17.675L16.4 4.075Q16.15 3.825 16.15 3.525Q16.15 3.225 16.375 2.975Q16.6 2.75 16.913 2.75Q17.225 2.75 17.425 2.95L19.925 5.475Q20.175 5.725 20.175 6.1Q20.175 6.475 19.925 6.725L17.45 9.2Q17.225 9.45 16.913 9.45Q16.6 9.45 16.375 9.2Q16.15 8.975 16.15 8.675Q16.15 8.375 16.375 8.15L17.675 6.85Z" />
    </svg>
  );
}

export function IconClose(props: { className?: string }) {
  return (
    <svg className={props.className} fill="currentColor" width="28" height="28" viewBox="0 0 24 24">
      <path d="M12 13.05 6.925 18.125Q6.725 18.325 6.413 18.337Q6.1 18.35 5.875 18.125Q5.65 17.9 5.65 17.6Q5.65 17.3 5.875 17.075L10.95 12L5.875 6.925Q5.675 6.725 5.663 6.412Q5.65 6.1 5.875 5.875Q6.1 5.65 6.4 5.65Q6.7 5.65 6.925 5.875L12 10.95L17.075 5.875Q17.275 5.675 17.588 5.662Q17.9 5.65 18.125 5.875Q18.35 6.1 18.35 6.4Q18.35 6.7 18.125 6.925L13.05 12L18.125 17.075Q18.325 17.275 18.337 17.587Q18.35 17.9 18.125 18.125Q17.9 18.35 17.6 18.35Q17.3 18.35 17.075 18.125Z" />
    </svg>
  );
}
